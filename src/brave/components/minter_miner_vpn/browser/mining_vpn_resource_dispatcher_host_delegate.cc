/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/minter_miner_vpn/browser/mining_vpn_resource_dispatcher_host_delegate.h"
#include "brave/components/minter_miner_vpn/browser/mining_vpn_resource_throttle.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/minter_miner_vpn/browser/mining_vpn_service.h"

using content::ResourceType;

MinerVPNResourceDispatcherHostDelegate::MinerVPNResourceDispatcherHostDelegate() {
  g_brave_browser_process->mining_vpn_service()->Start();
}

MinerVPNResourceDispatcherHostDelegate::~MinerVPNResourceDispatcherHostDelegate() {
}

void MinerVPNResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  ChromeResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    request, resource_context, resource_type, throttles);

  content::ResourceThrottle* throttle = MaybeCreateMinerVpnResourceThrottle(
    request, resource_type);
  throttles->push_back(base::WrapUnique(throttle));
}
