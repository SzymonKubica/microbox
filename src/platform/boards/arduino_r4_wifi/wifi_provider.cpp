#ifndef EMULATOR
#if defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#endif
#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif
#include "../../interface/wifi.hpp"

#include "arduino_secrets.hpp"

class ArduinoWifiProvider : public WifiProvider
{
      public:
        /**
         * Returns the WiFi data of the network that we are currently connected
         * to.
         */
        WifiData *get_wifi_data() override
        {
#if defined(ARDUINO_UNOR4_WIFI)
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

                return data;
#endif
#if defined(ARDUINO_ARCH_ESP32)
                WifiData *data = new WifiData();
                WiFi.BSSID(data->bssid);
                WiFi.macAddress(data->mac_address);
                data->rssi = WiFi.RSSI();
                // TODO: add data extraction
                data->encryption_type = 2;

                String ssid_str = WiFi.SSID();
                data->ssid = new char[ssid_str.length() + 1];
                strcpy((char *)data->ssid, ssid_str.c_str());
                return data;
#else
                return nullptr;
#endif
        }

        /**
         * Tries to connect to the network with the given ssid and password.
         * If successful, it will return a pointer to the WifiData struct with
         * the connection details. If failed, the optional will be empty.
         */
        std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) override
        {

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_ESP32)
                Serial.println("Starting network connection.");
                int status = WL_IDLE_STATUS;

#ifdef ARDUINO_UNOR4_WIFI
                if (WiFi.status() == WL_NO_MODULE) {
                        Serial.println(
                            "Communication with WiFi module failed!");
                        return std::nullopt;
                }

                String fv = WiFi.firmwareVersion();
                if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
                        Serial.println("Please upgrade the firmware");
                }
#endif

#ifdef ARDUINO_ARCH_ESP32
                // on esp32 begin is non-blocking an can only be done
                // once.
                status = WiFi.begin(ssid, password);
                unsigned long start = millis();
                while (WiFi.status() != WL_CONNECTED) {
                        if (millis() - start > 10000) {
                                Serial.println("Retrying...");
                                WiFi.disconnect(true);
                                delay(1000);
                                WiFi.begin(ssid, password);
                                start = millis();
                        }
                        delay(500);
                }
#endif

#ifdef ARDUINO_UNOR4_WIFI
                // attempt to connect to WiFi network:
                while (status != WL_CONNECTED) {
                        Serial.print("Attempting to connect to WPA SSID: ");
                        Serial.println(ssid);
                        // Connect to WPA/WPA2 network:
                        // on arduino the begin is blocking so we do it here
                        status = WiFi.begin(ssid, password);
                        // wait 10 seconds for connection only if not connected
                        if (status == WL_CONNECTED) {
                                break;
                        }
                        delay(500);
                }
#endif

                // you're connected now, so print out the data:
                Serial.println("You're connected to the network");
                return get_wifi_data();
#else
                return std::nullopt;
#endif
        }

        bool is_connected() override
        {
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_ARCH_ESP32)
                return WiFi.status() == WL_CONNECTED;
#else
                return false;
#endif
        }
};
#endif
