// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_JOB_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_JOB_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "components/download/public/common/download_request_handle_interface.h"
#include "content/browser/byte_stream.h"
#include "content/browser/download/download_file.h"
#include "content/common/content_export.h"

namespace content {

class DownloadItemImpl;

// DownloadJob lives on UI thread and subclasses implement actual download logic
// and interact with DownloadItemImpl.
// The base class is a friend class of DownloadItemImpl.
class CONTENT_EXPORT DownloadJob {
 public:
  DownloadJob(
      DownloadItemImpl* download_item,
      std::unique_ptr<download::DownloadRequestHandleInterface> request_handle);
  virtual ~DownloadJob();

  // Download operations.
  // TODO(qinmin): Remove the |callback| and |download_file_| parameter if
  // DownloadJob owns download file.
  void Start(DownloadFile* download_file_,
             const DownloadFile::InitializeCallback& callback,
             const download::DownloadItem::ReceivedSlices& received_slices);
  virtual void Cancel(bool user_cancel);
  virtual void Pause();
  virtual void Resume(bool resume_request);

  bool is_paused() const { return is_paused_; }

  // Returns whether the download is parallelizable. The download may not send
  // parallel requests as it can be disabled through flags.
  virtual bool IsParallelizable() const;

  // Cancel a particular request starts from |offset|, while the download is not
  // canceled. Used in parallel download.
  // TODO(xingliu): Remove this function if download job owns download file.
  virtual void CancelRequestWithOffset(int64_t offset);

  // Whether the download is save package.
  virtual bool IsSavePackageDownload() const;

 protected:
  // Callback from file thread when we initialize the DownloadFile.
  virtual void OnDownloadFileInitialized(
      const DownloadFile::InitializeCallback& callback,
      download::DownloadInterruptReason result);

  // Add an input stream to the download sink. Return false if we start to
  // destroy download file.
  bool AddInputStream(std::unique_ptr<DownloadManager::InputStream> stream,
                      int64_t offset,
                      int64_t length);

  DownloadItemImpl* download_item_;

  // Used to perform operations on network request.
  // Can be null on interrupted download.
  std::unique_ptr<download::DownloadRequestHandleInterface> request_handle_;

 private:
  // If the download progress is paused by the user.
  bool is_paused_;

  base::WeakPtrFactory<DownloadJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_JOB_H_