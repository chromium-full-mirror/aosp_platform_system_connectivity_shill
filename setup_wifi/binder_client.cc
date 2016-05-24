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

#include "shill/setup_wifi/binder_client.h"

#include <sysexits.h>

#include <android/system/connectivity/shill/IManager.h>
#include <base/bind.h>
#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <brillo/daemons/daemon.h>
#include <shill/key_value_store.h>
#include <utils/String16.h>
// TODO(samueltan): remove these includes once b/27270173 is resolved,
// and Manager is no longer reliant on D-Bus service constants.
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

using android::IBinder;
using android::os::PersistableBundle;
using android::sp;
using android::String16;
using android::system::connectivity::shill::IManager;
using android::system::connectivity::shill::IService;
using std::string;

namespace {
const int kTimeoutBetweenStateChecksMs = 100;
const char kManagerName[] = "android.system.connectivity.shill.IManager";
}  // namespace

namespace setup_wifi {

BinderClient::BinderClient(const string& ssid, const string& psk,
                           bool is_hex_ssid, int timeout)
    : ssid_(ssid), psk_(psk), is_hex_ssid_(is_hex_ssid), timeout_(timeout) {}

int BinderClient::OnInit() {
  int ret = Daemon::OnInit();
  if (ret != EX_OK) {
    return ret;
  }
  ConfigureAndConnect();
  // Timeout if we can't get online.
  brillo::MessageLoop::current()->PostDelayedTask(
      base::Bind(&BinderClient::Quit, base::Unretained(this)),
      base::TimeDelta::FromSeconds(timeout_));
  return EX_OK;
}

bool BinderClient::ConfigureAndConnect() {
  android::BinderWrapper::Create();
  if (!binder_watcher_.Init()) {
    LOG(ERROR) << "Binder connection failed.";
    return false;
  }

  sp<IBinder> manager_binder =
      android::BinderWrapper::Get()->GetService(kManagerName);
  if (!manager_binder.get()) {
    LOG(ERROR) << "Manager service not registered.";
    return false;
  }

  sp<IManager> manager = android::interface_cast<IManager>(manager_binder);
  if (!manager->ConfigureService(GetServiceConfig(), &shill_service_proxy_)
           .isOk()) {
    LOG(ERROR) << "Configure service failed.";
    return false;
  }

  if (!shill_service_proxy_->Connect().isOk()) {
    LOG(ERROR) << "Connect service failed.";
    return false;
  }

  PostCheckWifiStatusTask();
  return true;
}

void BinderClient::PostCheckWifiStatusTask() {
  LOG(INFO) << "Sleeping now. Will check wifi status in 100 ms.";
  brillo::MessageLoop::current()->PostDelayedTask(
      base::Bind(&BinderClient::QuitIfOnline, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kTimeoutBetweenStateChecksMs));
}

// Check if the device is online. If it is, quit.
void BinderClient::QuitIfOnline() {
  if (IsOnline())
    Quit();
  else
    PostCheckWifiStatusTask();
}

bool BinderClient::IsOnline() {
  int state;
  if (!shill_service_proxy_->GetState(&state).isOk()) {
    LOG(ERROR) << "Get state failed.";
    PostCheckWifiStatusTask();
    return false;
  }

  return state == android::system::connectivity::shill::IService::STATE_ONLINE;
}

PersistableBundle BinderClient::GetServiceConfig() {
  PersistableBundle configure_bundle;
  configure_bundle.putString(String16(shill::kTypeProperty),
                             String16(shill::kTypeWifi));
  if (is_hex_ssid_) {
    configure_bundle.putString(String16(shill::kWifiHexSsid),
                               String16(ssid_.c_str()));
  } else {
    configure_bundle.putString(String16(shill::kSSIDProperty),
                               String16(ssid_.c_str()));
  }
  if (!psk_.empty()) {
    configure_bundle.putString(String16(shill::kPassphraseProperty),
                               String16(psk_.c_str()));
    configure_bundle.putString(String16(shill::kSecurityProperty),
                               String16(shill::kSecurityPsk));
  }
  return configure_bundle;
}

}  // namespace setup_wifi
