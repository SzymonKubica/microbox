#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <Arduino.h>
#include <string>
#include "./wifi_provider.hpp"
#include "../../interface/wifi.hpp"
#include "../../../common/logging.hpp"

#define TAG "wifi_provider"

std::unique_ptr<WifiData> Esp32WifiProvider::get_wifi_data()
{
        std::unique_ptr<WifiData> data = std::make_unique<WifiData>();
        WiFi.BSSID(data->bssid);
        WiFi.macAddress(data->mac_address);
        data->rssi = WiFi.RSSI();
        data->encryption_type = (uint8_t)WiFi.encryptionType(0);

        String ssid_str = WiFi.SSID();
        data->ssid = new char[ssid_str.length() + 1];
        strcpy((char *)data->ssid, ssid_str.c_str());
        return data;
}

std::optional<std::unique_ptr<WifiData>>
Esp32WifiProvider::connect_to_network(const char *ssid, const char *password)
{

        LOG_INFO(TAG, "Starting network connection.");
        int status = WL_IDLE_STATUS;

        // on esp32 begin is non-blocking an can only be done
        // once.
        status = WiFi.begin(ssid, password);
        LOG_INFO(TAG, "Connecting to network %s with password %s", ssid,
                 password);
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

struct WifiConnectionContext {
        WifiProvider *provider;
        std::string ssid;
        std::string password;

      public:
        WifiConnectionContext(WifiProvider *provider, const char *ssid,
                              const char *password)
            : provider(provider), ssid(ssid), password(password)
        {
        }
};

void connect_to_wifi_task(void *parameter);
void Esp32WifiProvider::connect_to_network_async(const char *ssid,
                                                 const char *password)
{
        // We need a slightly larger stack because of the network stack that
        // is used to connect to the WiFi network.
        int stack_size = 16 * 1024; // 16 KB
        LOG_DEBUG(
            TAG,
            "Creating a new task to connect to WiFi network asynchronously...");
        WifiConnectionContext *params =
            new WifiConnectionContext(this, ssid, password);
        xTaskCreate(connect_to_wifi_task, // function
                    "Subroutine to connect to wifi", stack_size,
                    (void *)params, // parameter
                    1,              // priority
                    NULL            // task handle
        );
}
void connect_to_wifi_task(void *parameter)
{
        LOG_DEBUG(TAG, "Connecting to WiFi from a sub-task...");
        WifiConnectionContext *params = (WifiConnectionContext *)parameter;
        Esp32WifiProvider wifi_provider;

        const char *ssid = params->ssid.c_str();
        const char *password = params->password.c_str();

        auto maybe_data = wifi_provider.connect_to_network(ssid, password);
        if (maybe_data.has_value()) {
                LOG_INFO(TAG, "Connected to WiFi network: %s", ssid);
        } else {
                LOG_INFO(TAG, "Failed to connect to WiFi network: %s", ssid);
        }
        delete params;
        vTaskDelete(NULL);
}

bool Esp32WifiProvider::is_connected() { return WiFi.status() == WL_CONNECTED; }

#endif
