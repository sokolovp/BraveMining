#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <thread>

#include "base/files/file_path.h"
#include "brave/components/minter_miner_vpn/browser/base_miner_vpn_service.h"
#include "content/public/common/resource_type.h"



//class AdBlockClient;
class MiningVpnServiceTest;

namespace mining_vpn {

const std::string kVPNComponentName("Miner VPN");
const std::string kVPNComponentId("aojooleppoeijadhpcfjlefeicbhkcfi");//ogdodajpikijoaagaembonkhammapcde");

const std::string kVPNComponentBase64PublicKey =
   "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCPaJHie12YPiuVr7PcksNDbqGjTSgwxAouc+Ol0NCIHixB5vfiU9LaQCY54TEQ+7M4+MejuOzMzlqT/WOqkUrFUFWGyYB0BstwUoXwlG7D/KAPPwVruZuiHr6MG7T0ABlC8zfq1bHwHp75d9D26ZRba8nUKAGyOOWRA0wVG4PeBQIDAQAB"; //"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCoxK0jFIHNOv0c+/vY8EmzisMvUIcGJkmPVFDHGXPrdh+LS8b20ZRNWqbX1evQo+XlRBuLDec0uCxxaFzOYiniBclu1rsunGAULf/fL/sPdXXPyLmSX+QXBWLW60kdYPMFWusxha5vnuqMWs9gFhoXYKaVchDepYEFOyc5DkC4rQIDAQAB";
//    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs0qzJmHSgIiw7IGFCxij"
//    "1NnB5hJ5ZQ1LKW9htL4EBOaMJvmqaDs/wfq0nw/goBHWsqqkMBynRTu2Hxxirvdb"
//    "cugn1Goys5QKPgAvKwDHJp9jlnADWm5xQvPQ4GE1mK1/I3ka9cEOCzPW6GI+wGLi"
//    "VPx9VZrxHHsSBIJRaEB5Tyi5bj0CZ+kcfMnRTsXIBw3C6xJgCVKISQUkd8mawVvG"
//    "vqOhBOogCdb9qza5eJ1Cgx8RWKucFfaWWxKLOelCiBMT1Hm1znAoVBHG/blhJJOD"
//    "5HcH/heRrB4MvrE1J76WF3fvZ03aHVcnlLtQeiNNOZ7VbBDXdie8Nomf/QswbBGa"
//    "VwIDAQAB";

// The brave shields service in charge of ad-block checking and init.
class MiningVpnService : public BaseMinerVpnService {
 public:
   MiningVpnService();
   ~MiningVpnService() override;

  bool ShouldStartRequest(const GURL &url,
    content::ResourceType resource_type,
    const std::string& tab_host) override;

 protected:
  bool Init() override;
  void Cleanup() override;
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir) override;

 private:
  friend class ::MiningVpnServiceTest;
  static std::string g_vpn_component_id_;
  static std::string g_vpn_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);

  std::vector<unsigned char> buffer_;
 // std::unique_ptr<AdBlockClient> ad_block_client_;
  
  	  
  std::thread* oRecvThd;

};

// Creates the MiningVpnService
std::unique_ptr<MiningVpnService> MiningVpnServiceFactory();

} 
