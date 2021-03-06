/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FileWriterBase_h
#define FileWriterBase_h

#include "platform/heap/Handle.h"
#include <memory>

namespace blink {

class WebFileWriter;

class FileWriterBase : public GarbageCollectedMixin {
  USING_PRE_FINALIZER(FileWriterBase, Dispose);

 public:
  virtual ~FileWriterBase();
  void Initialize(std::unique_ptr<WebFileWriter>, long long length);

  long long position() const { return position_; }
  long long length() const { return length_; }

  void Trace(blink::Visitor* visitor) override {}

 protected:
  FileWriterBase();

  WebFileWriter* Writer() { return writer_.get(); }

  void SetPosition(long long position) { position_ = position; }

  void SetLength(long long length) { length_ = length; }

  void SeekInternal(long long position);

  void ResetWriter();

 private:
  void Dispose();

  std::unique_ptr<WebFileWriter> writer_;
  long long position_;
  long long length_;
};

}  // namespace blink

#endif  // FileWriterBase_h
