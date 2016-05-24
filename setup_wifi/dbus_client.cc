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

#include "shill/setup_wifi/dbus_client.h"

#include <sysexits.h>

#include <base/logging.h>
#include <brillo/any.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

using brillo::Any;
using std::map;
using std::string;

namespace {
static const char kOnlineState[] = "online";
static const int kTimeoutBetweenStateChecksMs = 100;
}  // namespace

namespace setup_wifi {

DBusClient::DBusClient(const string& ssid, const string& psk, bool is_hex_ssid,
                       int timeout)
    : ssid_(ssid), psk_(psk), is_hex_ssid_(is_hex_ssid), timeout_(timeout) {}

int DBusClient::OnInit() {
  int ret = DBusDaemon::OnInit();
  if (ret != EX_OK) {
    return ret;
  }
  ConfigureAndConnect();
  // Timeout if we can't get online.
  brillo::MessageLoop::current()->PostDelayedTask(
      base::Bind(&DBusClient::Quit, base::Unretained(this)),
      base::TimeDelta::FromSeconds(timeout_));
  return EX_OK;
}

bool DBusClient::ConfigureAndConnect() {
  std::unique_ptr<org::chromium::flimflam::ManagerProxy> shill_manager_proxy(
      new org::chromium::flimflam::ManagerProxy(bus_));

  dbus::ObjectPath created_service;
  brillo::ErrorPtr configure_error;
  if (!shill_manager_proxy->ConfigureService(
          GetServiceConfig(), &created_service, &configure_error)) {
    LOG(ERROR) << "Configure service failed";
    return false;
  }

  brillo::ErrorPtr connect_error;
  shill_service_proxy_ = std::unique_ptr<org::chromium::flimflam::ServiceProxy>(
      new org::chromium::flimflam::ServiceProxy(bus_, created_service));
  if (!shill_service_proxy_->Connect(&connect_error)) {
    LOG(ERROR) << "Connect service failed";
    return false;
  }

  PostCheckWifiStatusTask();
  return true;
}

void DBusClient::PostCheckWifiStatusTask() {
  LOG(INFO) << "Sleeping now. Will check wifi status in 100 ms.";
  brillo::MessageLoop::current()->PostDelayedTask(
      base::Bind(&DBusClient::QuitIfOnline, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kTimeoutBetweenStateChecksMs));
}

// Check if the device is online. If it is, quit.
void DBusClient::QuitIfOnline() {
  if (IsOnline())
    Quit();
  else
    PostCheckWifiStatusTask();
}

bool DBusClient::IsOnline() {
  brillo::VariantDictionary properties;
  if (!shill_service_proxy_->GetProperties(&properties, nullptr)) {
    LOG(ERROR) << "Cannot get properties.";
    PostCheckWifiStatusTask();
    return false;
  }
  auto property_it = properties.find(shill::kStateProperty);
  if (property_it == properties.end()) {
    PostCheckWifiStatusTask();
    return false;
  }

  std::string state = property_it->second.TryGet<std::string>();
  return state == kOnlineState;
}

map<string, Any> DBusClient::GetServiceConfig() {
  map<string, Any> configure_dict;
  configure_dict[shill::kTypeProperty] = shill::kTypeWifi;
  if (is_hex_ssid_) {
    configure_dict[shill::kWifiHexSsid] = ssid_;
  } else {
    configure_dict[shill::kSSIDProperty] = ssid_;
  }
  if (!psk_.empty()) {
    configure_dict[shill::kPassphraseProperty] = psk_;
    configure_dict[shill::kSecurityProperty] = shill::kSecurityPsk;
  }
  return configure_dict;
}

}  // namespace setup_wifi
