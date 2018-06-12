// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/channel_info.h"

#import <Foundation/Foundation.h>

#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#include "components/version_info/version_info.h"

namespace chrome {

std::string GetChannelString() {
#if defined(GOOGLE_CHROME_BUILD) || defined(BRAVE_CHROMIUM_BUILD)
  // Use the main Chrome application bundle and not the framework bundle.
  // Keystone keys don't live in the framework.
  NSBundle* bundle = base::mac::OuterBundle();
  NSString* channel = [bundle objectForInfoDictionaryKey:@"KSChannelID"];

  // Only ever return "", "unknown", "beta", "dev", or "canary" in a branded
  // build.

#if defined(BRAVE_CHROMIUM_BUILD)
  if (!channel) {
#else
  if (![bundle objectForInfoDictionaryKey:@"KSProductID"]) {
    // This build is not Keystone-enabled, it can't have a channel.
    channel = @"unknown";
  } else if (!channel) {
#endif
    // For the stable channel, KSChannelID is not set.
    channel = @"";
  } else if ([channel isEqual:@"beta"] ||
             [channel isEqual:@"dev"] ||
             [channel isEqual:@"canary"]) {
    // do nothing.
  } else {
    channel = @"unknown";
  }

  return base::SysNSStringToUTF8(channel);
#else
  return std::string();
#endif
}

version_info::Channel GetChannel() {
#if defined(GOOGLE_CHROME_BUILD) || defined(BRAVE_CHROMIUM_BUILD)
  std::string channel = GetChannelString();
  if (channel.empty()) {
    return version_info::Channel::STABLE;
  } else if (channel == "beta") {
    return version_info::Channel::BETA;
  } else if (channel == "dev") {
    return version_info::Channel::DEV;
  } else if (channel == "canary") {
    return version_info::Channel::CANARY;
  }
#endif

  return version_info::Channel::UNKNOWN;
}

}  // namespace chrome