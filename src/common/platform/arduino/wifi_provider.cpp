#include <Arduino_FreeRTOS.h>
#include <WiFiS3.h>
#include "../interface/wifi.hpp"

#include "arduino_secrets.hpp"

class ArduinoWifiProvider : WifiProvider
{
      public:
        /**
         * Returns the WiFi data of the network that we are currently connected
         * to.
         */
        WifiData *get_wifi_data() override
        {
                WifiData *data = new WifiData();

                WiFi.BSSID(data->bssid);
                WiFi.macAddress(data->mac_address);
                data->rssi = WiFi.RSSI();
                data->encryption_type = WiFi.encryptionType();

                // Getting the ssid is a bit tricky, we need to allocate the
                // string depending on the size of this SSID
                int ssid_length = strlen(WiFi.SSID());
                data->ssid = new char[ssid_length + 1];
                strcpy((char *)data->ssid, WiFi.SSID());

                // TODO: IPAddress ip = WiFi.localIP(); (not sure what this IP
                // address structure is)
                return data;
        }

        /**
         * Tries to connect to the network with the given ssid and password.
         * If successful, it will return a pointer to the WifiData struct with
         * the connection details. If failed, the optional will be empty.
         */
        std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) override
        {

                int status = WL_IDLE_STATUS;

                // check for the WiFi module:
                if (WiFi.status() == WL_NO_MODULE) {
                        Serial.println(
                            "Communication with WiFi module failed!");
                        return std::nullopt;
                }

                String fv = WiFi.firmwareVersion();
                if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
                        Serial.println("Please upgrade the firmware");
                }

                Serial.print("Attempting to connect to WPA SSID: ");
                Serial.println(ssid);

                // Connect to WPA/WPA2 network:
                status = WiFi.begin(ssid, password);

                while (status != WL_CONNECTED) {
                        if (status == WL_CONNECTED) {
                                break;
                        }
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                }

                // you're connected now, so print out the data:
                Serial.print("You're connected to the network");
                return get_wifi_data();
        }

        /**
         * Tries to connect to the network with the given ssid and password.
         * This is a non-blocking call that will return immediately. The caller
         * is responsbible for checking the connection status later by calling
         * the is_connected() method.
         */
        void connect_to_network_async(const char *ssid,
                                      const char *password) override
        {
                int status = WL_IDLE_STATUS;

                // check for the WiFi module:
                if (WiFi.status() == WL_NO_MODULE) {
                        Serial.println(
                            "Communication with WiFi module failed!");
                        return;
                }

                String fv = WiFi.firmwareVersion();
                if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
                        Serial.println("Please upgrade the firmware");
                }

                Serial.print("Attempting to connect to WPA SSID: ");
                Serial.println(ssid);
                // Connect to WPA/WPA2 network:
                status = WiFi.begin(ssid, password);
        }

        bool is_connected() override { return WiFi.status() == WL_CONNECTED; }
};
