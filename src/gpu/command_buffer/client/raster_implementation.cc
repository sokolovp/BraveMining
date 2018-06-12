// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <GLES3/gl3.h>
#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <sstream>
#include <string>
#include "base/atomic_sequence_num.h"
#include "base/bits.h"
#include "base/compiler_specific.h"
#include "base/numerics/safe_math.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "gpu/command_buffer/client/gpu_control.h"
#include "gpu/command_buffer/client/query_tracker.h"
#include "gpu/command_buffer/client/raster_cmd_helper.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/client/transfer_buffer.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"

#if defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_SINGLE_THREAD_CHECK() SingleThreadChecker checker(this);
#else  // !defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_SINGLE_THREAD_CHECK()
#endif  // defined(GPU_CLIENT_DEBUG)

// TODO(backer): Update APIs to always write to the destination? See below.
//
// Check that destination pointers point to initialized memory.
// When the context is lost, calling GL function has no effect so if destination
// pointers point to initialized memory it can often lead to crash bugs. eg.
//
// If it was up to us we'd just always write to the destination but the OpenGL
// spec defines the behavior of OpenGL functions, not us. :-(
#if defined(GPU_DCHECK)
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) GPU_DCHECK(v)
#define GPU_CLIENT_DCHECK(v) GPU_DCHECK(v)
#elif defined(DCHECK)
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) DCHECK(v)
#define GPU_CLIENT_DCHECK(v) DCHECK(v)
#else
#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(v) ASSERT(v)
#define GPU_CLIENT_DCHECK(v) ASSERT(v)
#endif

#define GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION(type, ptr) \
  GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(          \
      ptr &&                                                     \
      (ptr[0] == static_cast<type>(0) || ptr[0] == static_cast<type>(-1)));

#define GPU_CLIENT_VALIDATE_DESTINATION_OPTIONAL_INITALIZATION(type, ptr) \
  GPU_CLIENT_VALIDATE_DESTINATION_INITALIZATION_ASSERT(                   \
      !ptr ||                                                             \
      (ptr[0] == static_cast<type>(0) || ptr[0] == static_cast<type>(-1)));

using gpu::gles2::GLES2Util;

namespace gpu {
namespace raster {

RasterImplementation::SingleThreadChecker::SingleThreadChecker(
    RasterImplementation* raster_implementation)
    : raster_implementation_(raster_implementation) {
  CHECK_EQ(0, raster_implementation_->use_count_);
  ++raster_implementation_->use_count_;
}

RasterImplementation::SingleThreadChecker::~SingleThreadChecker() {
  --raster_implementation_->use_count_;
  CHECK_EQ(0, raster_implementation_->use_count_);
}

RasterImplementation::RasterImplementation(
    RasterCmdHelper* helper,
    TransferBufferInterface* transfer_buffer,
    bool bind_generates_resource,
    bool lose_context_when_out_of_memory,
    GpuControl* gpu_control)
    : ImplementationBase(helper, transfer_buffer, gpu_control),
      helper_(helper),
      active_texture_unit_(0),
      error_bits_(0),
      lose_context_when_out_of_memory_(lose_context_when_out_of_memory),
      use_count_(0),
      current_trace_stack_(0),
      capabilities_(gpu_control->GetCapabilities()),
      aggressively_free_resources_(false),
      lost_(false) {
  DCHECK(helper);
  DCHECK(transfer_buffer);
  DCHECK(gpu_control);

  std::stringstream ss;
  ss << std::hex << this;
  this_in_hex_ = ss.str();
}

gpu::ContextResult RasterImplementation::Initialize(
    const SharedMemoryLimits& limits) {
  TRACE_EVENT0("gpu", "RasterImplementation::Initialize");

  auto result = ImplementationBase::Initialize(limits);
  if (result != gpu::ContextResult::kSuccess) {
    return result;
  }

  texture_units_ = std::make_unique<TextureUnit[]>(
      capabilities_.max_combined_texture_image_units);

  return gpu::ContextResult::kSuccess;
}

RasterImplementation::~RasterImplementation() {
  // Make sure the queries are finished otherwise we'll delete the
  // shared memory (mapped_memory_) which will free the memory used
  // by the queries. The GPU process when validating that memory is still
  // shared will fail and abort (ie, it will stop running).
  WaitForCmd();

  query_tracker_.reset();

  // Make sure the commands make it the service.
  WaitForCmd();
}

RasterCmdHelper* RasterImplementation::helper() const {
  return helper_;
}

IdAllocator* RasterImplementation::GetIdAllocator(IdNamespaces namespace_id) {
  DCHECK_EQ(namespace_id, IdNamespaces::kQueries);
  return &query_id_allocator_;
}

void RasterImplementation::OnGpuControlLostContext() {
  OnGpuControlLostContextMaybeReentrant();

  // This should never occur more than once.
  DCHECK(!lost_context_callback_run_);
  lost_context_callback_run_ = true;
  if (!lost_context_callback_.is_null()) {
    std::move(lost_context_callback_).Run();
  }
}

void RasterImplementation::OnGpuControlLostContextMaybeReentrant() {
  {
    base::AutoLock hold(lost_lock_);
    lost_ = true;
  }
}

void RasterImplementation::OnGpuControlErrorMessage(const char* message,
                                                    int32_t id) {
  if (!error_message_callback_.is_null())
    error_message_callback_.Run(message, id);
}

void RasterImplementation::SetAggressivelyFreeResources(
    bool aggressively_free_resources) {
  TRACE_EVENT1("gpu", "RasterImplementation::SetAggressivelyFreeResources",
               "aggressively_free_resources", aggressively_free_resources);
  aggressively_free_resources_ = aggressively_free_resources;

  if (aggressively_free_resources_ && helper_->HaveRingBuffer()) {
    // Flush will delete transfer buffer resources if
    // |aggressively_free_resources_| is true.
    Flush();
  } else {
    ShallowFlushCHROMIUM();
  }
}

void RasterImplementation::Swap() {
  NOTREACHED();
}

void RasterImplementation::SwapWithBounds(const std::vector<gfx::Rect>&
                                          /* rects */) {
  NOTREACHED();
}

void RasterImplementation::PartialSwapBuffers(
    const gfx::Rect& /* sub_buffer */) {
  NOTREACHED();
}

void RasterImplementation::CommitOverlayPlanes() {
  NOTREACHED();
}

void RasterImplementation::ScheduleOverlayPlane(
    int /* plane_z_order */,
    gfx::OverlayTransform /* plane_transform */,
    unsigned /* overlay_texture_id */,
    const gfx::Rect& /* display_bounds */,
    const gfx::RectF& /* uv_rect */) {
  NOTREACHED();
}

uint64_t RasterImplementation::ShareGroupTracingGUID() const {
  NOTREACHED();
  return 0;
}

void RasterImplementation::SetErrorMessageCallback(
    base::RepeatingCallback<void(const char*, int32_t)> callback) {
  error_message_callback_ = std::move(callback);
}

void RasterImplementation::SetSnapshotRequested() {
  // Should only be called for real GL contexts.
  NOTREACHED();
}

bool RasterImplementation::ThreadSafeShallowLockDiscardableTexture(
    uint32_t texture_id) {
  return discardable_texture_manager_.TextureIsValid(texture_id) &&
         discardable_texture_manager_.LockTexture(texture_id);
}

void RasterImplementation::CompleteLockDiscardableTexureOnContextThread(
    uint32_t texture_id) {
  helper_->LockDiscardableTextureCHROMIUM(texture_id);
}

bool RasterImplementation::ThreadsafeDiscardableTextureIsDeletedForTracing(
    uint32_t texture_id) {
  return discardable_texture_manager_.TextureIsDeletedForTracing(texture_id);
}

const std::string& RasterImplementation::GetLogPrefix() const {
  const std::string& prefix(debug_marker_manager_.GetMarker());
  return prefix.empty() ? this_in_hex_ : prefix;
}

GLenum RasterImplementation::GetError() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGetError()");
  GLenum err = GetGLError();
  GPU_CLIENT_LOG("returned " << GLES2Util::GetStringError(err));
  return err;
}

void RasterImplementation::IssueBeginQuery(GLenum target,
                                           GLuint id,
                                           uint32_t sync_data_shm_id,
                                           uint32_t sync_data_shm_offset) {
  helper_->BeginQueryEXT(target, id, sync_data_shm_id, sync_data_shm_offset);
}

void RasterImplementation::IssueEndQuery(GLenum target, GLuint submit_count) {
  helper_->EndQueryEXT(target, submit_count);
}

void RasterImplementation::IssueQueryCounter(GLuint id,
                                             GLenum target,
                                             uint32_t sync_data_shm_id,
                                             uint32_t sync_data_shm_offset,
                                             GLuint submit_count) {
  NOTIMPLEMENTED();
}

void RasterImplementation::IssueSetDisjointValueSync(
    uint32_t sync_data_shm_id,
    uint32_t sync_data_shm_offset) {
  NOTIMPLEMENTED();
}

GLenum RasterImplementation::GetClientSideGLError() {
  if (error_bits_ == 0) {
    return GL_NO_ERROR;
  }

  GLenum error = GL_NO_ERROR;
  for (uint32_t mask = 1; mask != 0; mask = mask << 1) {
    if ((error_bits_ & mask) != 0) {
      error = GLES2Util::GLErrorBitToGLError(mask);
      break;
    }
  }
  error_bits_ &= ~GLES2Util::GLErrorToErrorBit(error);
  return error;
}

CommandBufferHelper* RasterImplementation::cmd_buffer_helper() {
  return helper_;
}

void RasterImplementation::IssueCreateTransferCacheEntry(
    GLuint entry_type,
    GLuint entry_id,
    GLuint handle_shm_id,
    GLuint handle_shm_offset,
    GLuint data_shm_id,
    GLuint data_shm_offset,
    GLuint data_size) {
  helper_->CreateTransferCacheEntryINTERNAL(entry_type, entry_id, handle_shm_id,
                                            handle_shm_offset, data_shm_id,
                                            data_shm_offset, data_size);
}

void RasterImplementation::IssueDeleteTransferCacheEntry(GLuint entry_type,
                                                         GLuint entry_id) {
  helper_->DeleteTransferCacheEntryINTERNAL(entry_type, entry_id);
}

void RasterImplementation::IssueUnlockTransferCacheEntry(GLuint entry_type,
                                                         GLuint entry_id) {
  helper_->UnlockTransferCacheEntryINTERNAL(entry_type, entry_id);
}

CommandBuffer* RasterImplementation::command_buffer() const {
  return helper_->command_buffer();
}

GLenum RasterImplementation::GetGLError() {
  TRACE_EVENT0("gpu", "RasterImplementation::GetGLError");
  // Check the GL error first, then our wrapped error.
  typedef cmds::GetError::Result Result;
  Result* result = GetResultAs<Result*>();
  // If we couldn't allocate a result the context is lost.
  if (!result) {
    return GL_NO_ERROR;
  }
  *result = GL_NO_ERROR;
  helper_->GetError(GetResultShmId(), GetResultShmOffset());
  WaitForCmd();
  GLenum error = *result;
  if (error == GL_NO_ERROR) {
    error = GetClientSideGLError();
  } else {
    // There was an error, clear the corresponding wrapped error.
    error_bits_ &= ~GLES2Util::GLErrorToErrorBit(error);
  }
  return error;
}

#if defined(RASTER_CLIENT_FAIL_GL_ERRORS)
void RasterImplementation::FailGLError(GLenum error) {
  if (error != GL_NO_ERROR) {
    NOTREACHED() << "Error";
  }
}
// NOTE: Calling GetGLError overwrites data in the result buffer.
void RasterImplementation::CheckGLError() {
  FailGLError(GetGLError());
}
#endif  // defined(RASTER_CLIENT_FAIL_GL_ERRORS)

void RasterImplementation::SetGLError(GLenum error,
                                      const char* function_name,
                                      const char* msg) {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] Client Synthesized Error: "
                     << GLES2Util::GetStringError(error) << ": "
                     << function_name << ": " << msg);
  FailGLError(error);
  if (msg) {
    last_error_ = msg;
  }
  if (!error_message_callback_.is_null()) {
    std::string temp(GLES2Util::GetStringError(error) + " : " + function_name +
                     ": " + (msg ? msg : ""));
    error_message_callback_.Run(temp.c_str(), 0);
  }
  error_bits_ |= GLES2Util::GLErrorToErrorBit(error);

  if (error == GL_OUT_OF_MEMORY && lose_context_when_out_of_memory_) {
    helper_->LoseContextCHROMIUM(GL_GUILTY_CONTEXT_RESET_ARB,
                                 GL_UNKNOWN_CONTEXT_RESET_ARB);
  }
}

void RasterImplementation::SetGLErrorInvalidEnum(const char* function_name,
                                                 GLenum value,
                                                 const char* label) {
  SetGLError(
      GL_INVALID_ENUM, function_name,
      (std::string(label) + " was " + GLES2Util::GetStringEnum(value)).c_str());
}

bool RasterImplementation::GetIntegervHelper(GLenum pname, GLint* params) {
  switch (pname) {
    case GL_ACTIVE_TEXTURE:
      *params = active_texture_unit_ + GL_TEXTURE0;
      return true;
    case GL_MAX_TEXTURE_SIZE:
      *params = capabilities_.max_texture_size;
      return true;
    case GL_TEXTURE_BINDING_2D:
      *params = texture_units_[active_texture_unit_].bound_texture_2d;
      return true;
    default:
      return false;
  }
}

bool RasterImplementation::GetQueryObjectValueHelper(const char* function_name,
                                                     GLuint id,
                                                     GLenum pname,
                                                     GLuint64* params) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] GetQueryObjectValueHelper(" << id
                     << ", " << GLES2Util::GetStringQueryObjectParameter(pname)
                     << ", " << static_cast<const void*>(params) << ")");

  gles2::QueryTracker::Query* query = query_tracker_->GetQuery(id);
  if (!query) {
    SetGLError(GL_INVALID_OPERATION, function_name, "unknown query id");
    return false;
  }

  if (query->Active()) {
    SetGLError(GL_INVALID_OPERATION, function_name,
               "query active. Did you call glEndQueryEXT?");
    return false;
  }

  if (query->NeverUsed()) {
    SetGLError(GL_INVALID_OPERATION, function_name,
               "Never used. Did you call glBeginQueryEXT?");
    return false;
  }

  bool valid_value = false;
  switch (pname) {
    case GL_QUERY_RESULT_EXT:
      if (!query->CheckResultsAvailable(helper_)) {
        helper_->WaitForToken(query->token());
        if (!query->CheckResultsAvailable(helper_)) {
          FinishHelper();
          CHECK(query->CheckResultsAvailable(helper_));
        }
      }
      *params = query->GetResult();
      valid_value = true;
      break;
    case GL_QUERY_RESULT_AVAILABLE_EXT:
      *params = query->CheckResultsAvailable(helper_);
      valid_value = true;
      break;
    default:
      SetGLErrorInvalidEnum(function_name, pname, "pname");
      break;
  }
  GPU_CLIENT_LOG("  " << *params);
  CheckGLError();
  return valid_value;
}

void RasterImplementation::Flush() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glFlush()");
  // Insert the cmd to call glFlush
  helper_->Flush();
  FlushHelper();
}

void RasterImplementation::IssueShallowFlush() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glShallowFlushCHROMIUM()");
  FlushHelper();
}

void RasterImplementation::ShallowFlushCHROMIUM() {
  IssueShallowFlush();
}

void RasterImplementation::FlushHelper() {
  // Flush our command buffer
  // (tell the service to execute up to the flush cmd.)
  helper_->CommandBufferHelper::Flush();

  if (aggressively_free_resources_)
    FreeEverything();
}

void RasterImplementation::OrderingBarrierCHROMIUM() {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glOrderingBarrierCHROMIUM");
  // Flush command buffer at the GPU channel level.  May be implemented as
  // Flush().
  helper_->CommandBufferHelper::OrderingBarrier();
}

void RasterImplementation::Finish() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  FinishHelper();
}

void RasterImplementation::FinishHelper() {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glFinish()");
  TRACE_EVENT0("gpu", "RasterImplementation::Finish");
  // Insert the cmd to call glFinish
  helper_->Finish();
  // Finish our command buffer
  // (tell the service to execute up to the Finish cmd and wait for it to
  // execute.)
  helper_->CommandBufferHelper::Finish();

  if (aggressively_free_resources_)
    FreeEverything();
}

void RasterImplementation::GenQueriesEXTHelper(GLsizei /* n */,
                                               const GLuint* /* queries */) {}

void RasterImplementation::DeleteTexturesHelper(GLsizei n,
                                                const GLuint* textures) {
  helper_->DeleteTexturesImmediate(n, textures);
  for (GLsizei ii = 0; ii < n; ++ii) {
    texture_id_allocator_.FreeID(textures[ii]);
    discardable_texture_manager_.FreeTexture(textures[ii]);
  }
  UnbindTexturesHelper(n, textures);
}

void RasterImplementation::UnbindTexturesHelper(GLsizei n,
                                                const GLuint* textures) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    for (GLint tt = 0; tt < capabilities_.max_combined_texture_image_units;
         ++tt) {
      TextureUnit& unit = texture_units_[tt];
      if (textures[ii] == unit.bound_texture_2d) {
        unit.bound_texture_2d = 0;
      }
    }
  }
}

GLenum RasterImplementation::GetGraphicsResetStatusKHR() {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGetGraphicsResetStatusKHR()");

  base::AutoLock hold(lost_lock_);
  if (lost_)
    return GL_UNKNOWN_CONTEXT_RESET_KHR;
  return GL_NO_ERROR;
}

void RasterImplementation::DeleteQueriesEXTHelper(GLsizei n,
                                                  const GLuint* queries) {
  IdAllocator* id_allocator = GetIdAllocator(IdNamespaces::kQueries);
  for (GLsizei ii = 0; ii < n; ++ii) {
    query_tracker_->RemoveQuery(queries[ii]);
    id_allocator->FreeID(queries[ii]);
  }

  helper_->DeleteQueriesEXTImmediate(n, queries);
}

void RasterImplementation::BeginQueryEXT(GLenum target, GLuint id) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] BeginQueryEXT("
                     << GLES2Util::GetStringQueryTarget(target) << ", " << id
                     << ")");

  switch (target) {
    case GL_COMMANDS_COMPLETED_CHROMIUM:
      if (!capabilities_.sync_query) {
        SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT",
                   "not enabled for commands completed queries");
        return;
      }
      break;
    default:
      SetGLError(GL_INVALID_ENUM, "glBeginQueryEXT", "unknown query target");
      return;
  }

  // if any outstanding queries INV_OP
  if (query_tracker_->GetCurrentQuery(target)) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT",
               "query already in progress");
    return;
  }

  if (id == 0) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT", "id is 0");
    return;
  }

  if (!GetIdAllocator(IdNamespaces::kQueries)->InUse(id)) {
    SetGLError(GL_INVALID_OPERATION, "glBeginQueryEXT", "invalid id");
    return;
  }

  if (query_tracker_->BeginQuery(id, target, this))
    CheckGLError();
}

void RasterImplementation::EndQueryEXT(GLenum target) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] EndQueryEXT("
                     << GLES2Util::GetStringQueryTarget(target) << ")");
  if (query_tracker_->EndQuery(target, this))
    CheckGLError();
}

void RasterImplementation::GetQueryObjectuivEXT(GLuint id,
                                                GLenum pname,
                                                GLuint* params) {
  GLuint64 result = 0;
  if (GetQueryObjectValueHelper("glGetQueryObjectuivEXT", id, pname, &result))
    *params = base::saturated_cast<GLuint>(result);
}

void RasterImplementation::GenSyncTokenCHROMIUM(GLbyte* sync_token) {
  if (!sync_token) {
    SetGLError(GL_INVALID_VALUE, "glGenSyncTokenCHROMIUM", "empty sync_token");
    return;
  }

  uint64_t fence_sync = gpu_control_->GenerateFenceSyncRelease();
  helper_->InsertFenceSyncCHROMIUM(fence_sync);
  helper_->CommandBufferHelper::OrderingBarrier();
  gpu_control_->EnsureWorkVisible();

  // Copy the data over after setting the data to ensure alignment.
  SyncToken sync_token_data(gpu_control_->GetNamespaceID(),
                            gpu_control_->GetCommandBufferID(), fence_sync);
  sync_token_data.SetVerifyFlush();
  memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
}

void RasterImplementation::GenUnverifiedSyncTokenCHROMIUM(GLbyte* sync_token) {
  if (!sync_token) {
    SetGLError(GL_INVALID_VALUE, "glGenUnverifiedSyncTokenCHROMIUM",
               "empty sync_token");
    return;
  }

  uint64_t fence_sync = gpu_control_->GenerateFenceSyncRelease();
  helper_->InsertFenceSyncCHROMIUM(fence_sync);
  helper_->CommandBufferHelper::OrderingBarrier();

  // Copy the data over after setting the data to ensure alignment.
  SyncToken sync_token_data(gpu_control_->GetNamespaceID(),
                            gpu_control_->GetCommandBufferID(), fence_sync);
  memcpy(sync_token, &sync_token_data, sizeof(sync_token_data));
}

void RasterImplementation::VerifySyncTokensCHROMIUM(GLbyte** sync_tokens,
                                                    GLsizei count) {
  bool requires_synchronization = false;
  for (GLsizei i = 0; i < count; ++i) {
    if (sync_tokens[i]) {
      SyncToken sync_token;
      memcpy(&sync_token, sync_tokens[i], sizeof(sync_token));

      if (sync_token.HasData() && !sync_token.verified_flush()) {
        if (!GetVerifiedSyncTokenForIPC(sync_token, &sync_token)) {
          SetGLError(GL_INVALID_VALUE, "glVerifySyncTokensCHROMIUM",
                     "Cannot verify sync token using this context.");
          return;
        }
        requires_synchronization = true;
        DCHECK(sync_token.verified_flush());
      }

      // Set verify bit on empty sync tokens too.
      sync_token.SetVerifyFlush();

      memcpy(sync_tokens[i], &sync_token, sizeof(sync_token));
    }
  }

  // Ensure all the fence syncs are visible on GPU service.
  if (requires_synchronization)
    gpu_control_->EnsureWorkVisible();
}

void RasterImplementation::WaitSyncTokenCHROMIUM(
    const GLbyte* sync_token_data) {
  if (!sync_token_data)
    return;

  // Copy the data over before data access to ensure alignment.
  SyncToken sync_token, verified_sync_token;
  memcpy(&sync_token, sync_token_data, sizeof(SyncToken));

  if (!sync_token.HasData())
    return;

  if (!GetVerifiedSyncTokenForIPC(sync_token, &verified_sync_token)) {
    SetGLError(GL_INVALID_VALUE, "glWaitSyncTokenCHROMIUM",
               "Cannot wait on sync_token which has not been verified");
    return;
  }

  helper_->WaitSyncTokenCHROMIUM(
      static_cast<GLint>(sync_token.namespace_id()),
      sync_token.command_buffer_id().GetUnsafeValue(),
      sync_token.release_count());

  // Enqueue sync token in flush after inserting command so that it's not
  // included in an automatic flush.
  gpu_control_->WaitSyncTokenHint(verified_sync_token);
}

namespace {

bool CreateImageValidInternalFormat(GLenum internalformat,
                                    const Capabilities& capabilities) {
  switch (internalformat) {
    case GL_ATC_RGB_AMD:
    case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
      return capabilities.texture_format_atc;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      return capabilities.texture_format_dxt1;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return capabilities.texture_format_dxt5;
    case GL_ETC1_RGB8_OES:
      return capabilities.texture_format_etc1;
    case GL_R16_EXT:
      return capabilities.texture_norm16;
    case GL_RGB10_A2_EXT:
      return capabilities.image_xr30;
    case GL_RED:
    case GL_RG_EXT:
    case GL_RGB:
    case GL_RGBA:
    case GL_RGB_YCBCR_422_CHROMIUM:
    case GL_RGB_YCBCR_420V_CHROMIUM:
    case GL_RGB_YCRCB_420_CHROMIUM:
    case GL_BGRA_EXT:
      return true;
    default:
      return false;
  }
}

}  // namespace

GLuint RasterImplementation::CreateImageCHROMIUMHelper(ClientBuffer buffer,
                                                       GLsizei width,
                                                       GLsizei height,
                                                       GLenum internalformat) {
  if (width <= 0) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "width <= 0");
    return 0;
  }

  if (height <= 0) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "height <= 0");
    return 0;
  }

  if (!CreateImageValidInternalFormat(internalformat, capabilities_)) {
    SetGLError(GL_INVALID_VALUE, "glCreateImageCHROMIUM", "invalid format");
    return 0;
  }

  // CreateImage creates a fence sync so we must flush first to ensure all
  // previously created fence syncs are flushed first.
  FlushHelper();

  int32_t image_id =
      gpu_control_->CreateImage(buffer, width, height, internalformat);
  if (image_id < 0) {
    SetGLError(GL_OUT_OF_MEMORY, "glCreateImageCHROMIUM", "image_id < 0");
    return 0;
  }
  return image_id;
}

GLuint RasterImplementation::CreateImageCHROMIUM(ClientBuffer buffer,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLenum internalformat) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glCreateImageCHROMIUM(" << width
                     << ", " << height << ", "
                     << GLES2Util::GetStringImageInternalFormat(internalformat)
                     << ")");
  GLuint image_id =
      CreateImageCHROMIUMHelper(buffer, width, height, internalformat);
  CheckGLError();
  return image_id;
}

void RasterImplementation::DestroyImageCHROMIUMHelper(GLuint image_id) {
  // Flush the command stream to make sure all pending commands
  // that may refer to the image_id are executed on the service side.
  helper_->CommandBufferHelper::Flush();
  gpu_control_->DestroyImage(image_id);
}

void RasterImplementation::DestroyImageCHROMIUM(GLuint image_id) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glDestroyImageCHROMIUM("
                     << image_id << ")");
  DestroyImageCHROMIUMHelper(image_id);
  CheckGLError();
}

void RasterImplementation::InitializeDiscardableTextureCHROMIUM(
    GLuint texture_id) {
  if (discardable_texture_manager_.TextureIsValid(texture_id)) {
    SetGLError(GL_INVALID_VALUE, "glInitializeDiscardableTextureCHROMIUM",
               "Texture ID already initialized");
    return;
  }
  ClientDiscardableHandle handle =
      discardable_texture_manager_.InitializeTexture(helper_->command_buffer(),
                                                     texture_id);
  if (!handle.IsValid())
    return;

  helper_->InitializeDiscardableTextureCHROMIUM(texture_id, handle.shm_id(),
                                                handle.byte_offset());
}

void RasterImplementation::UnlockDiscardableTextureCHROMIUM(GLuint texture_id) {
  if (!discardable_texture_manager_.TextureIsValid(texture_id)) {
    SetGLError(GL_INVALID_VALUE, "glUnlockDiscardableTextureCHROMIUM",
               "Texture ID not initialized");
    return;
  }

  // |should_unbind_texture| will be set to true if the texture has been fully
  // unlocked. In this case, ensure the texture is unbound.
  bool should_unbind_texture = false;
  discardable_texture_manager_.UnlockTexture(texture_id,
                                             &should_unbind_texture);
  if (should_unbind_texture)
    UnbindTexturesHelper(1, &texture_id);

  helper_->UnlockDiscardableTextureCHROMIUM(texture_id);
}

bool RasterImplementation::LockDiscardableTextureCHROMIUM(GLuint texture_id) {
  if (!discardable_texture_manager_.TextureIsValid(texture_id)) {
    SetGLError(GL_INVALID_VALUE, "glLockDiscardableTextureCHROMIUM",
               "Texture ID not initialized");
    return false;
  }
  if (!discardable_texture_manager_.LockTexture(texture_id)) {
    // Failure to lock means that this texture has been deleted on the service
    // side. Delete it here as well.
    DeleteTexturesHelper(1, &texture_id);
    return false;
  }
  helper_->LockDiscardableTextureCHROMIUM(texture_id);
  return true;
}

void* RasterImplementation::MapRasterCHROMIUM(GLsizeiptr size) {
  if (size < 0) {
    SetGLError(GL_INVALID_VALUE, "glMapRasterCHROMIUM", "negative size");
    return nullptr;
  }
  if (raster_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glMapRasterCHROMIUM", "already mapped");
    return nullptr;
  }
  raster_mapped_buffer_.emplace(size, helper_, transfer_buffer_);
  if (!raster_mapped_buffer_->valid()) {
    SetGLError(GL_INVALID_OPERATION, "glMapRasterCHROMIUM", "size too big");
    raster_mapped_buffer_ = base::nullopt;
    return nullptr;
  }
  return raster_mapped_buffer_->address();
}

void RasterImplementation::UnmapRasterCHROMIUM(GLsizeiptr written_size) {
  if (written_size < 0) {
    SetGLError(GL_INVALID_VALUE, "glUnmapRasterCHROMIUM",
               "negative written_size");
    return;
  }
  if (!raster_mapped_buffer_) {
    SetGLError(GL_INVALID_OPERATION, "glUnmapRasterCHROMIUM", "not mapped");
    return;
  }
  DCHECK(raster_mapped_buffer_->valid());
  if (written_size == 0) {
    raster_mapped_buffer_->Discard();
    raster_mapped_buffer_ = base::nullopt;
    return;
  }
  raster_mapped_buffer_->Shrink(written_size);
  helper_->RasterCHROMIUM(written_size, raster_mapped_buffer_->shm_id(),
                          raster_mapped_buffer_->offset());
  raster_mapped_buffer_ = base::nullopt;
  CheckGLError();
}

// Include the auto-generated part of this file. We split this because it means
// we can easily edit the non-auto generated parts right here in this file
// instead of having to edit some template or the code generator.
#include "gpu/command_buffer/client/raster_implementation_impl_autogen.h"

void RasterImplementation::GenTextures(GLsizei n, GLuint* textures) {
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glGenTextures(" << n << ", "
                     << static_cast<const void*>(textures) << ")");
  if (n < 0) {
    SetGLError(GL_INVALID_VALUE, "glGenTextures", "n < 0");
    return;
  }
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  for (int ii = 0; ii < n; ++ii) {
    textures[ii] = texture_id_allocator_.AllocateID();
  }
  // TODO(backer): Send some signal to service side.
  // helper_->GenTexturesImmediate(n, textures);
  // if (share_group_->bind_generates_resource())
  //   helper_->CommandBufferHelper::Flush();

  GPU_CLIENT_LOG_CODE_BLOCK({
    for (GLsizei i = 0; i < n; ++i) {
      GPU_CLIENT_LOG("  " << i << ": " << textures[i]);
    }
  });
  CheckGLError();
}

void RasterImplementation::BindTexture(GLenum target, GLuint texture) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glBindTexture("
                     << GLES2Util::GetStringEnum(texture) << ")");
  DCHECK_EQ(target, static_cast<GLenum>(GL_TEXTURE_2D));
  if (target != GL_TEXTURE_2D) {
    return;
  }
  TextureUnit& unit = texture_units_[active_texture_unit_];
  unit.bound_texture_2d = texture;
  // TODO(backer): Update bound texture on the server side.
  // helper_->BindTexture(target, texture);
  texture_id_allocator_.MarkAsUsed(texture);
}

void RasterImplementation::ActiveTexture(GLenum texture) {
  GPU_CLIENT_SINGLE_THREAD_CHECK();
  GPU_CLIENT_LOG("[" << GetLogPrefix() << "] glActiveTexture("
                     << GLES2Util::GetStringEnum(texture) << ")");
  GLuint texture_index = texture - GL_TEXTURE0;
  if (texture_index >=
      static_cast<GLuint>(capabilities_.max_combined_texture_image_units)) {
    SetGLErrorInvalidEnum("glActiveTexture", texture, "texture");
    return;
  }

  active_texture_unit_ = texture_index;
  // TODO(backer): Update active texture on the server side.
  // helper_->ActiveTexture(texture);
  CheckGLError();
}

void RasterImplementation::GenerateMipmap(GLenum target) {
  NOTIMPLEMENTED();
}
void RasterImplementation::SetColorSpaceMetadataCHROMIUM(
    GLuint texture_id,
    GLColorSpace color_space) {
  NOTIMPLEMENTED();
}
void RasterImplementation::GenMailboxCHROMIUM(GLbyte* mailbox) {
  NOTIMPLEMENTED();
}
void RasterImplementation::ProduceTextureDirectCHROMIUM(GLuint texture,
                                                        const GLbyte* mailbox) {
  NOTIMPLEMENTED();
}
GLuint RasterImplementation::CreateAndConsumeTextureCHROMIUM(
    const GLbyte* mailbox) {
  NOTIMPLEMENTED();
  return 0;
}
void RasterImplementation::BindTexImage2DCHROMIUM(GLenum target,
                                                  GLint imageId) {
  NOTIMPLEMENTED();
}
void RasterImplementation::ReleaseTexImage2DCHROMIUM(GLenum target,
                                                     GLint imageId) {
  NOTIMPLEMENTED();
}
void RasterImplementation::TexImage2D(GLenum target,
                                      GLint level,
                                      GLint internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLint border,
                                      GLenum format,
                                      GLenum type,
                                      const void* pixels) {
  NOTIMPLEMENTED();
}
void RasterImplementation::TexSubImage2D(GLenum target,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type,
                                         const void* pixels) {
  NOTIMPLEMENTED();
}
void RasterImplementation::CompressedTexImage2D(GLenum target,
                                                GLint level,
                                                GLenum internalformat,
                                                GLsizei width,
                                                GLsizei height,
                                                GLint border,
                                                GLsizei imageSize,
                                                const void* data) {
  NOTIMPLEMENTED();
}
void RasterImplementation::TexStorageForRaster(GLenum target,
                                               viz::ResourceFormat format,
                                               GLsizei width,
                                               GLsizei height,
                                               RasterTexStorageFlags flags) {
  NOTIMPLEMENTED();
}
void RasterImplementation::CopySubTextureCHROMIUM(
    GLuint source_id,
    GLint source_level,
    GLenum dest_target,
    GLuint dest_id,
    GLint dest_level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLboolean unpack_flip_y,
    GLboolean unpack_premultiply_alpha,
    GLboolean unpack_unmultiply_alpha) {
  NOTIMPLEMENTED();
}
void RasterImplementation::BeginRasterCHROMIUM(
    GLuint texture_id,
    GLuint sk_color,
    GLuint msaa_sample_count,
    GLboolean can_use_lcd_text,
    GLboolean use_distance_field_text,
    GLint pixel_config,
    const cc::RasterColorSpace& raster_color_space) {
  NOTIMPLEMENTED();
}
void RasterImplementation::RasterCHROMIUM(const cc::DisplayItemList* list,
                                          cc::ImageProvider* provider,
                                          const gfx::Size& content_size,
                                          const gfx::Rect& full_raster_rect,
                                          const gfx::Rect& playback_rect,
                                          const gfx::Vector2dF& post_translate,
                                          GLfloat post_scale,
                                          bool requires_clear) {
  NOTIMPLEMENTED();
}
void RasterImplementation::BeginGpuRaster() {
  NOTIMPLEMENTED();
}
void RasterImplementation::EndGpuRaster() {
  NOTIMPLEMENTED();
}

}  // namespace raster
}  // namespace gpu