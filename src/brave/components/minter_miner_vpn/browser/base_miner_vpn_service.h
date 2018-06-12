#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"
#include "brave/components/brave_shields/browser/base_brave_shields_service.h"


namespace mining_vpn {

class BaseMinerVpnService : public ComponentsUI {
 public:
  BaseMinerVpnService(const std::string& component_name,
                          const std::string& component_id,
                          const std::string& component_base64_public_key);
  virtual ~BaseMinerVpnService();
  bool Start();
  void Stop();
  bool IsInitialized() const;
  virtual bool ShouldStartRequest(const GURL& url,
      content::ResourceType resource_type,
      const std::string& tab_host);
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

 protected:
  virtual bool Init() = 0;
  virtual void Cleanup() = 0;
  virtual void OnComponentRegistered(const std::string& component_id);
  virtual void OnComponentReady(const std::string& component_id,
                                const base::FilePath& install_dir);

 private:
  void InitShields();

  bool initialized_;
  std::mutex initialized_mutex_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::string component_name_;
  std::string component_id_;
  std::string component_base64_public_key_;
};

}

