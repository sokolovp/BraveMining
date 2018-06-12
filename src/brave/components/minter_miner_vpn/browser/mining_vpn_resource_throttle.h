#pragma once

#include "base/macros.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"


namespace net {
struct RedirectInfo;
class URLRequest;
}

// Contructs a resource throttle for Brave shields like tracking protection
// and adblock. It returns a
content::ResourceThrottle* MaybeCreateMinerVpnResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type);

// This check is done before requesting the original URL, and additionally
// before following any subsequent redirect.
class MinerVpnResourceThrottle
    : public content::ResourceThrottle {
 private:
  friend content::ResourceThrottle* MaybeCreateMinerVpnResourceThrottle(
      net::URLRequest* request,
      content::ResourceType resource_type);

  MinerVpnResourceThrottle(net::URLRequest* request,
                               content::ResourceType resource_type);

  ~MinerVpnResourceThrottle() override;

  // content::ResourceThrottle:
  void WillStartRequest(bool* defer) override;
  const char* GetNameForLogging() const override;

  net::URLRequest* request_;
  content::ResourceType resource_type_;

  DISALLOW_COPY_AND_ASSIGN(MinerVpnResourceThrottle);
};

