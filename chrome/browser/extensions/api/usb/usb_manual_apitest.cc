// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"

namespace {

class UsbManualApiTest : public ExtensionApiTest {
 public:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableExperimentalExtensionApis);
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(UsbManualApiTest, MANUAL_ListInterfaces) {
  extensions::PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  extensions::PermissionsRequestFunction::SetAutoConfirmForTests(true);
  ASSERT_TRUE(RunExtensionTest("usb_manual/list_interfaces"));
}
