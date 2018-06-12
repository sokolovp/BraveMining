#pragma once 

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class PrefRegistrySimple;

namespace mining_vpn {

class MiningVpnWebContentsObserver : public content::WebContentsObserver,
    public content::WebContentsUserData<MiningVpnWebContentsObserver> {
 public:
  MiningVpnWebContentsObserver(content::WebContents*);
  ~MiningVpnWebContentsObserver() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void DispatchBlockedEventForWebContents(
      const std::string& block_type,
      const std::string& subresource,
      content::WebContents* web_contents);
  static void DispatchBlockedEvent(
      const std::string& block_type,
      const std::string& subresource,
      int render_process_id,
      int render_frame_id, int frame_tree_node_id);
  static GURL GetTabURLFromRenderFrameInfo(int render_process_id, int render_frame_id);

 protected:
    // A set of identifiers that uniquely identifies a RenderFrame.
  struct RenderFrameIdKey {
    RenderFrameIdKey();
    RenderFrameIdKey(int render_process_id, int frame_routing_id);

    // The process ID of the renderer that contains the RenderFrame.
    int render_process_id;

    // The routing ID of the RenderFrame.
    int frame_routing_id;

    bool operator<(const RenderFrameIdKey& other) const;
    bool operator==(const RenderFrameIdKey& other) const;
  };

  // content::WebContentsObserver overrides.
  void RenderFrameCreated(content::RenderFrameHost* host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;

  // Invoked if an IPC message is coming from a specific RenderFrameHost.
  bool OnMessageReceived(const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) override;
  void OnJavaScriptBlockedWithDetail(
      content::RenderFrameHost* render_frame_host,
      const base::string16& details);
  void OnFingerprintingBlockedWithDetail(
      content::RenderFrameHost* render_frame_host,
      const base::string16& details);

  static std::map<RenderFrameIdKey, GURL> render_frame_key_to_tab_url;
  // This lock protects |frame_data_map_| from being concurrently written on the
  // UI thread and read on the IO thread.
  static base::Lock frame_data_map_lock_;
  DISALLOW_COPY_AND_ASSIGN(MiningVpnWebContentsObserver);
};

}  
