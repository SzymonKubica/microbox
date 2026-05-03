#if defined(ARDUINO_ARCH_ESP32)
#pragma once
#include <WiFi.h>
#include <Arduino.h>
#include "../../interface/wifi.hpp"

class Esp32WifiProvider : public WifiProvider
{
      public:
        /**
         * Returns a unique pointer to the newly created WiFi data of the
         * network that we are currently connected to.
         */
        std::unique_ptr<WifiData> get_wifi_data() override;
        /**
         * Tries to connect to the network with the given ssid and password.
         * If successful, it will return a pointer to the WifiData struct with
         * the connection details. If failed, the optional will be empty. This
         * is a blocking call that will ont return unitl connection is
         * established or failed.
         */
        std::optional<std::unique_ptr<WifiData>>
        connect_to_network(const char *ssid, const char *password) override;
        bool is_connected() override;
};
#endif
