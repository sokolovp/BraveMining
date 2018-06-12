/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

#include "brave/common/webui_url_constants.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "chrome/common/url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "url/gurl.h"

using content::WebUI;
using content::WebUIController;

namespace {

// A function for creating a new WebUI. The caller owns the return value, which
// may be NULL (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template<class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

template<>
WebUIController* NewWebUI<BasicUI>(WebUI* web_ui, const GURL& url) {
  auto host = url.host_piece();
  if (host == kPaymentsHost) {
    return new BasicUI(web_ui, url.host(), kPaymentsJS,
        IDR_BRAVE_PAYMENTS_JS, IDR_BRAVE_PAYMENTS_HTML);
  } else if (host ==  chrome::kChromeUINewTabHost) {
    return new BasicUI(web_ui, url.host(), kBraveNewTabJS,
        IDR_BRAVE_NEW_TAB_JS, IDR_BRAVE_NEW_TAB_HTML);
  }
  return nullptr;
}

// Returns a function that can be used to create the right type of WebUI for a
// tab, based on its URL. Returns NULL if the URL doesn't have WebUI associated
// with it.
WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui,
                                             const GURL& url) {
  if (url.host_piece() == kPaymentsHost ||
      url.host_piece() ==  chrome::kChromeUINewTabHost) {
    return &NewWebUI<BasicUI>;
  }

  return nullptr;
}

}  // namespace

WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const {

  WebUIFactoryFunction function = GetWebUIFactoryFunction(NULL, url);
  if (function) {
    return reinterpret_cast<WebUI::TypeID>(function);
  }
  return ChromeWebUIControllerFactory::GetWebUIType(browser_context, url);
}

WebUIController* BraveWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {

  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, url);
  if (!function) {
    return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(
        web_ui, url);
  }

  return (*function)(web_ui, url);
}


// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  return base::Singleton<BraveWebUIControllerFactory>::get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() {
}

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() {
}

