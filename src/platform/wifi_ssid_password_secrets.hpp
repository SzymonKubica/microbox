#pragma once
// To set the default wifi ssid & password you need to build the console with
// -DWIFI_SSID and -DWIFI_PASSWORD build flags. You can see the compile command
// under `scripts/compile-target.sh`
#if !defined(WIFI_SSID)
#define WIFI_SSID "ssid"
#endif
#if !defined(WIFI_PASSWORD)
#define WIFI_PASSWORD "password"
#endif
