#pragma once 

#include <memory>
#include <vector>

#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"


class MinerVPNResourceDispatcherHostDelegate
    : public ChromeResourceDispatcherHostDelegate {
 public:
  MinerVPNResourceDispatcherHostDelegate();
  ~MinerVPNResourceDispatcherHostDelegate() override;

 protected:
  void AppendStandardResourceThrottles(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::ResourceType resource_type,
      std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles)
    override;

  DISALLOW_COPY_AND_ASSIGN(MinerVPNResourceDispatcherHostDelegate);
};
