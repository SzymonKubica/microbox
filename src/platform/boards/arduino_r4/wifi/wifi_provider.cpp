#if defined(ARDUINO_UNOR4_WIFI)
#include <Arduino.h>
#include "wifi_provider.hpp"
#include "../../../../common/logging.hpp"
#define TAG "wifi_provider"
WifiData *ArduinoWifiProvider::get_wifi_data()
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

        return data;
}

std::optional<WifiData *>
ArduinoWifiProvider::connect_to_network(const char *ssid, const char *password)
{

        LOG_INFO(TAG, "Starting network connection.");
        int status = WL_IDLE_STATUS;

        if (WiFi.status() == WL_NO_MODULE) {
                LOG_INFO(TAG, "Communication with WiFi module failed!");
                return std::nullopt;
        }

        String fv = WiFi.firmwareVersion();
        if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
                LOG_INFO(TAG, "Please upgrade the firmware");
        }

        // attempt to connect to WiFi network:
        while (status != WL_CONNECTED) {
                LOG_INFO(TAG, "Attempting to connect to WPA SSID: ");
                LOG_INFO(TAG, ssid);
                // Connect to WPA/WPA2 network:
                // on arduino the begin is blocking so we do it here
                status = WiFi.begin(ssid, password);
                // wait 10 seconds for connection only if not connected
                if (status == WL_CONNECTED) {
                        break;
                }
                delay(500);
        }

        // you're connected now, so print out the data:
        LOG_INFO(TAG, "You're connected to the network");
        return get_wifi_data();
}

bool ArduinoWifiProvider::is_connected()
{
        return WiFi.status() == WL_CONNECTED;
}
#endif
