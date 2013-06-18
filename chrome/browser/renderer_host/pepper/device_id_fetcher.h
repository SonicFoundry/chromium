// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_instance.h"

class Profile;

namespace content {
class BrowserPpapiHost;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chrome {

class PepperFlashDeviceIDHost;

// This class allows asynchronously fetching a unique device ID. The callback
// passed in on creation will be called when the ID has been fetched.
class DeviceIDFetcher : public base::RefCountedThreadSafe<DeviceIDFetcher> {
 public:
  typedef base::Callback<void(const std::string&)> IDCallback;

  explicit DeviceIDFetcher(const IDCallback& callback, PP_Instance instance);

  // Schedules the request operation.
  void Start(content::BrowserPpapiHost* browser_host);

  // Called to register the |kEnableDRM| and |kDRMSalt| preferences.
  static void RegisterUserPrefs(user_prefs::PrefRegistrySyncable* prefs);

  // Return the path where the legacy device ID is stored (for ChromeOS only).
  static base::FilePath GetLegacyDeviceIDPath(
      const base::FilePath& profile_path);

 private:
  ~DeviceIDFetcher();

  // Checks the preferences for DRM (whether DRM is enabled and getting the drm
  // salt) on the UI thread. These are passed to |ComputeOnIOThread|.
  void CheckPrefsOnUIThread(int process_id);

  // Compute the device ID on the IO thread with the given salt.
  void ComputeOnIOThread(const std::string& salt);
  // Legacy method used to get the device ID for ChromeOS.
  void ComputeOnBlockingPool(const base::FilePath& profile_path,
                             const std::string& salt);

  // Calls back into the PepperFlashDeviceIDHost on the IO thread with the
  // device ID or the empty string on failure.
  void RunCallbackOnIOThread(const std::string& id);

  // Helper which returns an ID unique to the system. Returns an empty string if
  // the call fails.
  std::string GetMachineID();

  friend class base::RefCountedThreadSafe<DeviceIDFetcher>;

  // The callback to run when the ID has been fetched.
  IDCallback callback_;

  PP_Instance instance_;

  DISALLOW_COPY_AND_ASSIGN(DeviceIDFetcher);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_DEVICE_ID_FETCHER_H_
