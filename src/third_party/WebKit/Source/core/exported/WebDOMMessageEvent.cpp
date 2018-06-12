/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "public/web/WebDOMMessageEvent.h"

#include "bindings/core/v8/serialization/SerializedScriptValue.h"
#include "core/dom/Document.h"
#include "core/events/MessageEvent.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/messaging/BlinkTransferableMessage.h"
#include "core/messaging/MessagePort.h"
#include "public/platform/WebString.h"
#include "public/web/WebDocument.h"
#include "public/web/WebFrame.h"
#include "public/web/WebSerializedScriptValue.h"

namespace blink {

WebDOMMessageEvent::WebDOMMessageEvent(
    const WebSerializedScriptValue& message_data,
    const WebString& origin,
    const WebFrame* source_frame,
    const WebDocument& target_document,
    WebVector<MessagePortChannel> channels)
    : WebDOMMessageEvent(MessageEvent::Create()) {
  DOMWindow* window = nullptr;
  if (source_frame)
    window = WebFrame::ToCoreFrame(*source_frame)->DomWindow();
  MessagePortArray* ports = nullptr;
  if (!target_document.IsNull()) {
    Document* core_document = target_document;
    ports = MessagePort::EntanglePorts(*core_document, std::move(channels));
  }
  // TODO(esprehn): Chromium always passes empty string for lastEventId, is that
  // right?
  Unwrap<MessageEvent>()->initMessageEvent("message", false, false,
                                           message_data, origin,
                                           "" /*lastEventId*/, window, ports);
}

WebDOMMessageEvent::WebDOMMessageEvent(TransferableMessage message,
                                       const WebString& origin,
                                       const WebFrame* source_frame,
                                       const WebDocument& target_document)
    : WebDOMMessageEvent(MessageEvent::Create()) {
  DOMWindow* window = nullptr;
  if (source_frame)
    window = WebFrame::ToCoreFrame(*source_frame)->DomWindow();
  BlinkTransferableMessage msg = ToBlinkTransferableMessage(std::move(message));
  MessagePortArray* ports = nullptr;
  if (!target_document.IsNull()) {
    Document* core_document = target_document;
    ports = MessagePort::EntanglePorts(*core_document, std::move(msg.ports));
  }
  // TODO(esprehn): Chromium always passes empty string for lastEventId, is that
  // right?
  Unwrap<MessageEvent>()->initMessageEvent("message", false, false,
                                           std::move(msg.message), origin,
                                           "" /*lastEventId*/, window, ports);
}

WebString WebDOMMessageEvent::Origin() const {
  return WebString(ConstUnwrap<MessageEvent>()->origin());
}

TransferableMessage WebDOMMessageEvent::AsMessage() {
  BlinkTransferableMessage msg;
  msg.message = Unwrap<MessageEvent>()->DataAsSerializedScriptValue();
  msg.ports = Unwrap<MessageEvent>()->ReleaseChannels();
  return ToTransferableMessage(std::move(msg));
}

}  // namespace blink