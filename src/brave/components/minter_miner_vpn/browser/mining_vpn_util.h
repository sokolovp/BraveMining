#pragma once

#include <stdint.h>
#include <string>

#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"


namespace net {
class URLRequest;
}

namespace mining_vpn {

bool IsAllowContentSettingFromIO(net::URLRequest* request,
    GURL primary_url, GURL secondary_url, ContentSettingsType setting_type,
    const std::string& resource_identifier);

void DispatchBlockedEventFromIO(net::URLRequest* request,
    const std::string& block_type);

} 
