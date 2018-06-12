/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

namespace brave {

void ShowBravePayments(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIPaymentsURL)));
}

}  // namespace brave
