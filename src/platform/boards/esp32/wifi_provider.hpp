#if defined(ARDUINO_ARCH_ESP32)
#pragma once
#include <WiFi.h>
#include <Arduino.h>
#include "../../interface/wifi.hpp"

class Esp32WifiProvider : public WifiProvider
{
      public:
        /**
         * Returns the WiFi data of the network that we are currently connected
         * to.
         */
        WifiData *get_wifi_data() override;
        std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) override;
        bool is_connected() override;
};
#endif
