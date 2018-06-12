// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/effective_connection_type_helper.h"

namespace content {

#define STATIC_ASSERT_ENUM(a, b)                            \
  static_assert(static_cast<int>(a) == static_cast<int>(b), \
                "mismatching enums: " #a)

STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN,
                   blink::WebEffectiveConnectionType::kTypeUnknown);
STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
                   blink::WebEffectiveConnectionType::kTypeOffline);
STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
                   blink::WebEffectiveConnectionType::kTypeSlow2G);
STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_2G,
                   blink::WebEffectiveConnectionType::kType2G);
STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_3G,
                   blink::WebEffectiveConnectionType::kType3G);
STATIC_ASSERT_ENUM(net::EFFECTIVE_CONNECTION_TYPE_4G,
                   blink::WebEffectiveConnectionType::kType4G);

#undef STATIC_ASSERT_ENUM

blink::WebEffectiveConnectionType
EffectiveConnectionTypeToWebEffectiveConnectionType(
    net::EffectiveConnectionType net_type) {
  return static_cast<blink::WebEffectiveConnectionType>(net_type);
}

}  // namespace content