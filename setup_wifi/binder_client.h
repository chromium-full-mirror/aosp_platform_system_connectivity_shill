//
// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_SETUP_WIFI_BINDER_CLIENT_H_
#define SHILL_SETUP_WIFI_BINDER_CLIENT_H_

#include <map>
#include <string>

#include <android/system/connectivity/shill/IService.h>
#include <binder/PersistableBundle.h>
#include <brillo/binder_watcher.h>
#include <brillo/daemons/daemon.h>
#include <utils/StrongPointer.h>

namespace setup_wifi {

class BinderClient : public brillo::Daemon {
 public:
  BinderClient(const std::string& ssid, const std::string& psk,
               bool is_hex_ssid, int timeout);
  ~BinderClient() override {}

 protected:
  int OnInit() override;

 private:
  bool ConfigureAndConnect();
  void PostCheckWifiStatusTask();
  void QuitIfOnline();
  bool IsOnline();
  android::os::PersistableBundle GetServiceConfig();

  brillo::BinderWatcher binder_watcher_;
  android::sp<android::system::connectivity::shill::IService>
      shill_service_proxy_;
  std::string ssid_;
  std::string psk_;
  bool is_hex_ssid_;
  int timeout_;
};

}  // namespace setup_wifi

#endif  // SHILL_SETUP_WIFI_BINDER_CLIENT_H_
