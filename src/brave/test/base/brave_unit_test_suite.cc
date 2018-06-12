/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_unit_test_suite.h"

#include "base/logging.h"
#include "brave/common/brave_paths.h"
#include "chrome/test/base/chrome_unit_test_suite.h"

BraveUnitTestSuite::BraveUnitTestSuite(int argc, char** argv)
    : ChromeUnitTestSuite(argc, argv) {}

void BraveUnitTestSuite::Initialize() {
  ChromeUnitTestSuite::Initialize();

  brave::RegisterPathProvider();
}
