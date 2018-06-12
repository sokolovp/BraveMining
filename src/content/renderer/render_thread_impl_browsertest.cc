// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_thread_impl.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/location.h"
#include "base/memory/discardable_memory.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/app/mojo/mojo_init.h"
#include "content/common/in_process_child_thread_params.h"
#include "content/common/service_manager/child_connection.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_content_client_initializer.h"
#include "content/public/test/test_launcher.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_service_manager_context.h"
#include "content/renderer/render_process_impl.h"
#include "content/shell/browser/shell.h"
#include "content/test/mock_render_process.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/config/gpu_switches.h"
#include "gpu/ipc/host/gpu_switches.h"
#include "ipc/ipc.mojom.h"
#include "ipc/ipc_channel_mojo.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/scheduler/renderer/renderer_scheduler.h"
#include "third_party/WebKit/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gfx/buffer_format_util.h"

// IPC messages for testing ----------------------------------------------------

// TODO(mdempsky): Fix properly by moving into a separate
// browsertest_message_generator.cc file.
#undef IPC_IPC_MESSAGE_MACROS_H_
#undef IPC_MESSAGE_EXTRA
#define IPC_MESSAGE_IMPL
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_templates_impl.h"

#undef IPC_MESSAGE_START
#define IPC_MESSAGE_START TestMsgStart
IPC_MESSAGE_CONTROL0(TestMsg_QuitRunLoop)

// -----------------------------------------------------------------------------

// These tests leak memory, this macro disables the test when under the
// LeakSanitizer.
#ifdef LEAK_SANITIZER
#define WILL_LEAK(NAME) DISABLED_##NAME
#else
#define WILL_LEAK(NAME) NAME
#endif

namespace content {
namespace {

// FIXME: It would be great if there was a reusable mock SingleThreadTaskRunner
class TestTaskCounter : public base::SingleThreadTaskRunner {
 public:
  TestTaskCounter() : count_(0) {}

  // SingleThreadTaskRunner implementation.
  bool PostDelayedTask(const base::Location&,
                       base::OnceClosure,
                       base::TimeDelta) override {
    base::AutoLock auto_lock(lock_);
    count_++;
    return true;
  }

  bool PostNonNestableDelayedTask(const base::Location&,
                                  base::OnceClosure,
                                  base::TimeDelta) override {
    base::AutoLock auto_lock(lock_);
    count_++;
    return true;
  }

  bool RunsTasksInCurrentSequence() const override { return true; }

  int NumTasksPosted() const {
    base::AutoLock auto_lock(lock_);
    return count_;
  }

 private:
  ~TestTaskCounter() override {}

  mutable base::Lock lock_;
  int count_;
};

#if defined(COMPILER_MSVC)
// See explanation for other RenderViewHostImpl which is the same issue.
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

class RenderThreadImplForTest : public RenderThreadImpl {
 public:
  RenderThreadImplForTest(
      const InProcessChildThreadParams& params,
      std::unique_ptr<blink::scheduler::RendererScheduler> scheduler,
      scoped_refptr<base::SingleThreadTaskRunner>& test_task_counter)
      : RenderThreadImpl(params, std::move(scheduler), test_task_counter) {}

  ~RenderThreadImplForTest() override {}
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

class QuitOnTestMsgFilter : public IPC::MessageFilter {
 public:
  explicit QuitOnTestMsgFilter(base::OnceClosure quit_closure)
      : origin_task_runner_(
            blink::scheduler::GetSequencedTaskRunnerForTesting()),
        quit_closure_(std::move(quit_closure)) {}

  // IPC::MessageFilter overrides:
  bool OnMessageReceived(const IPC::Message& message) override {
    origin_task_runner_->PostTask(FROM_HERE, std::move(quit_closure_));
    return true;
  }

  bool GetSupportedMessageClasses(
      std::vector<uint32_t>* supported_message_classes) const override {
    supported_message_classes->push_back(TestMsgStart);
    return true;
  }

 private:
  ~QuitOnTestMsgFilter() override {}

  scoped_refptr<base::SequencedTaskRunner> origin_task_runner_;
  base::OnceClosure quit_closure_;
};

class RenderThreadImplBrowserTest : public testing::Test {
 public:
  RenderThreadImplBrowserTest() : field_trial_list_(nullptr) {}

  void SetUp() override {
    content_renderer_client_.reset(new ContentRendererClient());
    SetRendererClientForTesting(content_renderer_client_.get());

    browser_threads_.reset(
        new TestBrowserThreadBundle(TestBrowserThreadBundle::IO_MAINLOOP));
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
        blink::scheduler::GetSingleThreadTaskRunnerForTesting();

    InitializeMojo();
    shell_context_.reset(new TestServiceManagerContext);
    mojo::edk::OutgoingBrokerClientInvitation invitation;
    service_manager::Identity child_identity(
        mojom::kRendererServiceName, service_manager::mojom::kInheritUserID,
        "test");
    child_connection_.reset(new ChildConnection(
        child_identity, &invitation,
        ServiceManagerConnection::GetForProcess()->GetConnector(),
        io_task_runner));

    mojo::MessagePipe pipe;
    child_connection_->BindInterface(IPC::mojom::ChannelBootstrap::Name_,
                                     std::move(pipe.handle1));

    channel_ = IPC::ChannelProxy::Create(
        IPC::ChannelMojo::CreateServerFactory(
            std::move(pipe.handle0), io_task_runner,
            blink::scheduler::GetSingleThreadTaskRunnerForTesting()),
        nullptr, io_task_runner,
        blink::scheduler::GetSingleThreadTaskRunnerForTesting());

    mock_process_.reset(new MockRenderProcess);
    test_task_counter_ = base::MakeRefCounted<TestTaskCounter>();

    // RenderThreadImpl expects the browser to pass these flags.
    base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
    base::CommandLine::StringVector old_argv = cmd->argv();

    cmd->AppendSwitchASCII(switches::kLang, "en-US");

    cmd->AppendSwitchASCII(switches::kNumRasterThreads, "1");

    // To avoid creating a GPU channel to query if
    // accelerated_video_decode is blacklisted on older Android system
    // in RenderThreadImpl::Init().
    cmd->AppendSwitch(switches::kIgnoreGpuBlacklist);

    std::unique_ptr<blink::scheduler::RendererScheduler> renderer_scheduler =
        blink::scheduler::RendererScheduler::Create();
    scoped_refptr<base::SingleThreadTaskRunner> test_task_counter(
        test_task_counter_.get());

    base::FieldTrialList::CreateTrialsFromCommandLine(
        *cmd, switches::kFieldTrialHandle, -1);
    thread_ = new RenderThreadImplForTest(
        InProcessChildThreadParams(io_task_runner, &invitation,
                                   child_connection_->service_token()),
        std::move(renderer_scheduler), test_task_counter);
    cmd->InitFromArgv(old_argv);

    run_loop_ = std::make_unique<base::RunLoop>();
    test_msg_filter_ = base::MakeRefCounted<QuitOnTestMsgFilter>(
        run_loop_->QuitWhenIdleClosure());
    thread_->AddFilter(test_msg_filter_.get());
  }

  void TearDown() override {
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            kSingleProcessTestsFlag)) {
      // In a single-process mode, we need to avoid destructing mock_process_
      // because it will call _exit(0) and kill the process before the browser
      // side is ready to exit.
      ANNOTATE_LEAKING_OBJECT_PTR(mock_process_.get());
      mock_process_.release();
    }
  }

  IPC::Sender* sender() { return channel_.get(); }

  scoped_refptr<TestTaskCounter> test_task_counter_;
  TestContentClientInitializer content_client_initializer_;
  std::unique_ptr<ContentRendererClient> content_renderer_client_;

  std::unique_ptr<TestBrowserThreadBundle> browser_threads_;
  std::unique_ptr<TestServiceManagerContext> shell_context_;
  std::unique_ptr<ChildConnection> child_connection_;
  std::unique_ptr<IPC::ChannelProxy> channel_;

  std::unique_ptr<MockRenderProcess> mock_process_;
  scoped_refptr<QuitOnTestMsgFilter> test_msg_filter_;
  RenderThreadImplForTest* thread_;  // Owned by mock_process_.

  base::FieldTrialList field_trial_list_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

class RenderThreadImplMojoInputMessagesDisabledBrowserTest
    : public RenderThreadImplBrowserTest {
 public:
  RenderThreadImplMojoInputMessagesDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kMojoInputMessages);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

void CheckRenderThreadInputHandlerManager(RenderThreadImpl* thread) {
  ASSERT_TRUE(thread->input_handler_manager());
}

// Check that InputHandlerManager outlives compositor thread because it uses
// raw pointers to post tasks.
// Disabled under LeakSanitizer due to memory leaks. http://crbug.com/348994
// Disabled on Windows due to flakiness: http://crbug.com/728034.
#if defined(OS_WIN)
#define MAYBE_InputHandlerManagerDestroyedAfterCompositorThread \
  DISABLED_InputHandlerManagerDestroyedAfterCompositorThread
#else
#define MAYBE_InputHandlerManagerDestroyedAfterCompositorThread \
  InputHandlerManagerDestroyedAfterCompositorThread
#endif
TEST_F(RenderThreadImplMojoInputMessagesDisabledBrowserTest,
       WILL_LEAK(MAYBE_InputHandlerManagerDestroyedAfterCompositorThread)) {
  ASSERT_TRUE(thread_->input_handler_manager());

  thread_->compositor_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CheckRenderThreadInputHandlerManager, thread_));
}

// Disabled under LeakSanitizer due to memory leaks.
TEST_F(RenderThreadImplBrowserTest,
       WILL_LEAK(NonResourceDispatchIPCTasksDontGoThroughScheduler)) {
  // NOTE other than not being a resource message, the actual message is
  // unimportant.

  sender()->Send(new TestMsg_QuitRunLoop());

  run_loop_->Run();

  EXPECT_EQ(0, test_task_counter_->NumTasksPosted());
}

enum NativeBufferFlag { kDisableNativeBuffers, kEnableNativeBuffers };

class RenderThreadImplGpuMemoryBufferBrowserTest
    : public ContentBrowserTest,
      public testing::WithParamInterface<
          ::testing::tuple<NativeBufferFlag, gfx::BufferFormat>> {
 public:
  RenderThreadImplGpuMemoryBufferBrowserTest() {}
  ~RenderThreadImplGpuMemoryBufferBrowserTest() override {}

  gpu::GpuMemoryBufferManager* memory_buffer_manager() {
    return memory_buffer_manager_;
  }

 private:
  void SetUpOnRenderThread() {
    memory_buffer_manager_ =
        RenderThreadImpl::current()->GetGpuMemoryBufferManager();
  }

  // Overridden from BrowserTestBase:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kSingleProcess);
    NativeBufferFlag native_buffer_flag = ::testing::get<0>(GetParam());
    switch (native_buffer_flag) {
      case kEnableNativeBuffers:
        command_line->AppendSwitch(switches::kEnableNativeGpuMemoryBuffers);
        break;
      case kDisableNativeBuffers:
        command_line->AppendSwitch(switches::kDisableNativeGpuMemoryBuffers);
        break;
    }
  }

  void SetUpOnMainThread() override {
    NavigateToURL(shell(), GURL(url::kAboutBlankURL));
    PostTaskToInProcessRendererAndWait(base::Bind(
        &RenderThreadImplGpuMemoryBufferBrowserTest::SetUpOnRenderThread,
        base::Unretained(this)));
  }

  gpu::GpuMemoryBufferManager* memory_buffer_manager_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(RenderThreadImplGpuMemoryBufferBrowserTest);
};

// https://crbug.com/652531
IN_PROC_BROWSER_TEST_P(RenderThreadImplGpuMemoryBufferBrowserTest,
                       DISABLED_Map) {
  gfx::BufferFormat format = ::testing::get<1>(GetParam());
  gfx::Size buffer_size(4, 4);

  std::unique_ptr<gfx::GpuMemoryBuffer> buffer =
      memory_buffer_manager()->CreateGpuMemoryBuffer(
          buffer_size, format, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE,
          gpu::kNullSurfaceHandle);
  ASSERT_TRUE(buffer);
  EXPECT_EQ(format, buffer->GetFormat());

  // Map buffer planes.
  ASSERT_TRUE(buffer->Map());

  // Write to buffer and check result.
  size_t num_planes = gfx::NumberOfPlanesForBufferFormat(format);
  for (size_t plane = 0; plane < num_planes; ++plane) {
    ASSERT_TRUE(buffer->memory(plane));
    ASSERT_TRUE(buffer->stride(plane));
    size_t row_size_in_bytes =
        gfx::RowSizeForBufferFormat(buffer_size.width(), format, plane);
    EXPECT_GT(row_size_in_bytes, 0u);

    std::unique_ptr<char[]> data(new char[row_size_in_bytes]);
    memset(data.get(), 0x2a + plane, row_size_in_bytes);
    size_t height = buffer_size.height() /
                    gfx::SubsamplingFactorForBufferFormat(format, plane);
    for (size_t y = 0; y < height; ++y) {
      // Copy |data| to row |y| of |plane| and verify result.
      memcpy(
          static_cast<char*>(buffer->memory(plane)) + y * buffer->stride(plane),
          data.get(), row_size_in_bytes);
      EXPECT_EQ(0, memcmp(static_cast<char*>(buffer->memory(plane)) +
                              y * buffer->stride(plane),
                          data.get(), row_size_in_bytes));
    }
  }

  buffer->Unmap();
}

INSTANTIATE_TEST_CASE_P(
    RenderThreadImplGpuMemoryBufferBrowserTests,
    RenderThreadImplGpuMemoryBufferBrowserTest,
    ::testing::Combine(::testing::Values(kDisableNativeBuffers,
                                         kEnableNativeBuffers),
                       // These formats are guaranteed to work on all platforms.
                       ::testing::Values(gfx::BufferFormat::R_8,
                                         gfx::BufferFormat::BGR_565,
                                         gfx::BufferFormat::RGBA_4444,
                                         gfx::BufferFormat::RGBA_8888,
                                         gfx::BufferFormat::BGRA_8888,
                                         gfx::BufferFormat::YVU_420)));

class RenderThreadImplClearMemoryBrowserTest : public ContentBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }
};

#if defined(OS_ANDROID) || defined(OS_MACOSX) || defined(IS_CHROMECAST)
#define MAYBE_ClearMemory DISABLED_ClearMemory
#else
#define MAYBE_ClearMemory ClearMemory
#endif

IN_PROC_BROWSER_TEST_F(RenderThreadImplClearMemoryBrowserTest,
                       MAYBE_ClearMemory) {
  TestNavigationObserver observer(shell()->web_contents());
  GURL url(embedded_test_server()->GetURL("/simple_page.html"));
  NavigateToURL(shell(), url);

  EXPECT_GT(static_cast<int>(SkGraphics::GetFontCacheUsed()), 0);
  EXPECT_GT(SkGraphics::GetFontCacheCountUsed(), 0);

  // TODO(gyuyoung): How to call RenderThreadImpl::ClearMemory() from here?
  // Instead we call same function that RenderThreadImpl::ClearMemory() calls at
  // the moment.
  SkGraphics::PurgeAllCaches();

  EXPECT_EQ(static_cast<int>(SkGraphics::GetFontCacheUsed()), 0);
  EXPECT_EQ(SkGraphics::GetFontCacheCountUsed(), 0);
}

}  // namespace
}  // namespace content