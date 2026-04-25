#if defined(ARDUINO_UNOR4_WIFI)
#pragma once
#include <WiFiS3.h>
#include "../../interface/wifi.hpp"

#include "../../wifi_ssid_password_secrets.hpp"

class ArduinoWifiProvider : public WifiProvider
{
      public:
        /**
         * Returns the WiFi data of the network that we are currently connected
         * to.
         */
        WifiData *get_wifi_data() override;

        /**
         * Tries to connect to the network with the given ssid and password.
         * If successful, it will return a pointer to the WifiData struct with
         * the connection details. If failed, the optional will be empty.
         */
        std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) override;

        bool is_connected() override;
};
#endif
