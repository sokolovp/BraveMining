/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_system_network_delegate.h"

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"


BraveSystemNetworkDelegate::BraveSystemNetworkDelegate(
    extensions::EventRouterForwarder* event_router,
    BooleanPrefMember* enable_referrers) :
    BraveNetworkDelegateBase(event_router, enable_referrers) {
  brave::OnBeforeURLRequestCallback callback =
      base::Bind(
          brave::OnBeforeURLRequest_StaticRedirectWork);
  before_url_request_callbacks_.push_back(callback);
}

BraveSystemNetworkDelegate::~BraveSystemNetworkDelegate() {
}
