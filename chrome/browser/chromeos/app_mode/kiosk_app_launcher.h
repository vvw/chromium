// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_LAUNCHER_H_
#define CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_LAUNCHER_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

class Profile;

namespace chromeos {

// KioskAppLauncher launches a given app from login screen. It first attempts
// to mount a cryptohome for the app. If the mount is successful, it prepares
// app profile then calls StartupAppLauncher to finish the launch. Note that
// there should only be one launch attempt in progress.
class KioskAppLauncher {
 public:
  // Callback after a launch attempt.
  typedef base::Callback<void(bool success)> LaunchCallback;

  // Constructs a launcher for |app_id|. |callback| will be invoked to report
  // whether the launch attempt is success or not.
  KioskAppLauncher(const std::string& app_id,
                   const LaunchCallback& callback);
  ~KioskAppLauncher();

  // Starts a launch attempt. Fails immediately if there is already a launch
  // attempt running.
  void Start();

  bool success() const { return success_; }

 private:
  class CryptohomedChecker;
  class ProfileLoader;

  void ReportLaunchResult(bool success);

  void StartMount();
  void MountCallback(bool mount_success, cryptohome::MountError mount_error);

  void OnProfilePrepared(Profile* profile);

  // The instance of the current running launch.
  static KioskAppLauncher* running_instance_;

  const std::string app_id_;
  const LaunchCallback callback_;

  scoped_ptr<CryptohomedChecker> crytohomed_checker;
  scoped_ptr<ProfileLoader> profile_loader_;

  // True when cryptohome for the app is mounted successfully and restart
  // is scheduled.
  bool success_;

  DISALLOW_COPY_AND_ASSIGN(KioskAppLauncher);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_LAUNCHER_H_
