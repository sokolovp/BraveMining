/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/minter_miner_vpn/browser/mining_vpn_resource_throttle.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/minter_miner_vpn/browser/mining_vpn_service.h"
#include "brave/components/minter_miner_vpn/browser/mining_vpn_util.h"
#include "brave/components/minter_miner_vpn/common/mining_vpn_constants.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "net/url_request/url_request.h"


content::ResourceThrottle* MaybeCreateMinerVpnResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type) {
  return new MinerVpnResourceThrottle(request,
      resource_type);
}

MinerVpnResourceThrottle::MinerVpnResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type) :
      request_(request),
      resource_type_(resource_type) {
}

MinerVpnResourceThrottle::~MinerVpnResourceThrottle() = default;

const char* MinerVpnResourceThrottle::GetNameForLogging() const {
  return "MinerVpnResourceThrottle";
}

void MinerVpnResourceThrottle::WillStartRequest(bool* defer) {
  GURL tab_origin = request_->site_for_cookies().GetOrigin();
  // Proper content settings can't be looked up, so do nothing.
  if (tab_origin.is_empty()) {
    return;
  }
  
  //bool allow_mining_vpn = mining_vpn::IsAllowContentSettingFromIO(
      //request_, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      //mining_vpn::kBraveShields);
  /*bool allow_ads = mining_vpn::IsAllowContentSettingFromIO(
      request_, tab_origin, tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS, mining_vpn::kAds);
  bool allow_trackers = mining_vpn::IsAllowContentSettingFromIO(
      request_, tab_origin, tab_origin,
      CONTENT_SETTINGS_TYPE_PLUGINS, mining_vpn::kTrackers);
      */
  //if (allow_mining_vpn &&
    //  !allow_trackers &&
    //  !
    g_brave_browser_process->mining_vpn_service()->ShouldStartRequest(request_->url(), resource_type_, tab_origin.host());//) {
    //Cancel();
    //mining_vpn::DispatchBlockedEventFromIO(request_,
      //  mining_vpn::kTrackers);
  //}
  /*
  if (allow_mining_vpn &&
      !allow_ads &&
      !g_brave_browser_process->ad_block_service()->
      ShouldStartRequest(request_->url(), resource_type_, tab_origin.host())) {
    Cancel();
    mining_vpn::DispatchBlockedEventFromIO(request_,
        mining_vpn::kAds);
  }*/
}
