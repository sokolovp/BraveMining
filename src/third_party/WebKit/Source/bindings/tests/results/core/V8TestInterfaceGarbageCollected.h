// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/interface.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef V8TestInterfaceGarbageCollected_h
#define V8TestInterfaceGarbageCollected_h

#include "bindings/core/v8/GeneratedCodeHelper.h"
#include "bindings/core/v8/NativeValueTraits.h"
#include "bindings/core/v8/ToV8ForCore.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8EventTarget.h"
#include "bindings/tests/idls/core/TestInterfaceGarbageCollected.h"
#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/V8DOMWrapper.h"
#include "platform/bindings/WrapperTypeInfo.h"
#include "platform/heap/Handle.h"

namespace blink {

class V8TestInterfaceGarbageCollected {
  STATIC_ONLY(V8TestInterfaceGarbageCollected);
 public:
  CORE_EXPORT static bool hasInstance(v8::Local<v8::Value>, v8::Isolate*);
  static v8::Local<v8::Object> findInstanceInPrototypeChain(v8::Local<v8::Value>, v8::Isolate*);
  CORE_EXPORT static v8::Local<v8::FunctionTemplate> domTemplate(v8::Isolate*, const DOMWrapperWorld&);
  static TestInterfaceGarbageCollected* ToImpl(v8::Local<v8::Object> object) {
    return ToScriptWrappable(object)->ToImpl<TestInterfaceGarbageCollected>();
  }
  CORE_EXPORT static TestInterfaceGarbageCollected* ToImplWithTypeCheck(v8::Isolate*, v8::Local<v8::Value>);
  CORE_EXPORT static const WrapperTypeInfo wrapperTypeInfo;
  static const int internalFieldCount = kV8DefaultWrapperInternalFieldCount;

  // Callback functions
  CORE_EXPORT static void constructorCallback(const v8::FunctionCallbackInfo<v8::Value>&);

  CORE_EXPORT static void attr1AttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void attr1AttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void sizeAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);

  CORE_EXPORT static void funcMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void keysMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void entriesMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void forEachMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void hasMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void addMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void clearMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void deleteMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void iteratorMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);

  static void InstallRuntimeEnabledFeaturesOnTemplate(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::FunctionTemplate> interface_template);
};

template <>
struct NativeValueTraits<TestInterfaceGarbageCollected> : public NativeValueTraitsBase<TestInterfaceGarbageCollected> {
  CORE_EXPORT static TestInterfaceGarbageCollected* NativeValue(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&);
  CORE_EXPORT static TestInterfaceGarbageCollected* NullValue() { return nullptr; }
};

template <>
struct V8TypeOf<TestInterfaceGarbageCollected> {
  typedef V8TestInterfaceGarbageCollected Type;
};

}  // namespace blink

#endif  // V8TestInterfaceGarbageCollected_h