//
// Copyright (C) 2015 The Android Open Source Project
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

#include <sysexits.h>

#include <map>
#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <brillo/daemons/daemon.h>

#include "shill/setup_wifi/dbus_client.h"

namespace {
static const char kHelp[] = "help";
static const char kPassphrase[] = "passphrase";
static const char kHexSsid[] = "hex-ssid";
static const char kSSID[] = "ssid";
static const char kTimeOut[] = "wait-for-online-seconds";
static const char kHelpMessage[] =
    "\n"
    "Available Switches: \n"
    "  --ssid=<ssid>\n"
    "    Set the SSID to configure (mandatory).\n"
    "  --hex-ssid\n"
    "    SSID is provided in hexadecimal\n"
    "  --passphrase=<passprhase>\n"
    "    Set the passphrase for PSK networks\n"
    "  --wait-for-online-seconds=<seconds>\n"
    "    Number of seconds to wait to connect the SSID\n";
}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();

  if (cl->HasSwitch(kHelp)) {
    LOG(INFO) << kHelpMessage;
    return EXIT_SUCCESS;
  }

  if (!cl->HasSwitch(kSSID)) {
    LOG(ERROR) << "ssid switch is mandatory.";
    LOG(ERROR) << kHelpMessage;
    return EXIT_FAILURE;
  }

  std::string ssid = cl->GetSwitchValueASCII(kSSID);
  std::string psk;
  if (cl->HasSwitch(kPassphrase)) {
    psk = cl->GetSwitchValueASCII(kPassphrase);
  }
  bool is_hex_ssid = cl->HasSwitch(kHexSsid);

  int timeout = 0;
  if (cl->HasSwitch(kTimeOut)) {
    auto value = cl->GetSwitchValueASCII(kTimeOut);
    if (!base::StringToInt(value, &timeout)) {
      LOG(ERROR) << "Timeout value invalid";
      return EXIT_FAILURE;
    }
  }

  std::unique_ptr<brillo::Daemon> client(
      new setup_wifi::DBusClient(ssid, psk, is_hex_ssid, timeout));
  client->Run();
  LOG(INFO) << "Process exiting.";

  return EXIT_SUCCESS;
}

