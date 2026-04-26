#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <Arduino.h>
#include "./wifi_provider.hpp"
#include "../../interface/wifi.hpp"
#include "../../../common/logging.hpp"

#define TAG "wifi_provider"

WifiData *Esp32WifiProvider::get_wifi_data()
{
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
}

std::optional<WifiData *>
Esp32WifiProvider::connect_to_network(const char *ssid, const char *password)
{

        LOG_INFO(TAG, "Starting network connection.");
        int status = WL_IDLE_STATUS;

        // on esp32 begin is non-blocking an can only be done
        // once.
        status = WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
                if (millis() - start > 10000) {
                        LOG_INFO(TAG, "Retrying...");
                        WiFi.disconnect(true);
                        delay(1000);
                        WiFi.begin(ssid, password);
                        start = millis();
                }
                delay(500);
        }

        // you're connected now, so print out the data:
        LOG_INFO(TAG, "You're connected to the network");
        return get_wifi_data();
}

bool Esp32WifiProvider::is_connected() { return WiFi.status() == WL_CONNECTED; }
#endif
