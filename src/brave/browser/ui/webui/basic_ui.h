/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_controller.h"


class PrefChangeRegistrar;

class BasicUI : public content::WebUIController {
 public:
  explicit BasicUI(content::WebUI* web_ui, const std::string& host,
      const std::string& js_file, int js_resource_id, int html_resource_id);
  ~BasicUI() override;

 private:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BasicUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
