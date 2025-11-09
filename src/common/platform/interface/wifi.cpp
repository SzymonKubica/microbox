#include "wifi.hpp"
#include "stdio.h"

/**
 * Converts the given WifiData struct into a human-readable string. Can be
 * useful for logging.
 */
char *get_wifi_data_string(WifiData *data)
{
        // Allocate a buffer to hold the formatted string
        static char buffer[512];
        for (int i = 0; i < sizeof(buffer); i++) {
                buffer[i] = 0;
        }

        // Format the MAC address
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 data->mac_address[0], data->mac_address[1],
                 data->mac_address[2], data->mac_address[3],
                 data->mac_address[4], data->mac_address[5]);

        // Format the BSSID
        char bssid_str[18];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 data->bssid[0], data->bssid[1], data->bssid[2], data->bssid[3],
                 data->bssid[4], data->bssid[5]);

        // Create the final formatted string
        snprintf(buffer, sizeof(buffer),
                 "SSID: %s\r\nBSSID: %s\r\nMAC Address: %s\r\nRSSI: %ld "
                 "dBm\r\nEncryption Type: %u",
                 data->ssid, bssid_str, mac_str, data->rssi,
                 data->encryption_type);

        return buffer;
}

/**
 * Converts the given WifiData struct into a human-readable single line
 * string. This is useful for displaying in the actual game console as that one
 * performs custom string wrapping and does not support \r\n.
 */
char *get_wifi_data_string_single_line(WifiData *data)
{
        // Allocate a buffer to hold the formatted string
        static char buffer[512];
        for (int i = 0; i < sizeof(buffer); i++) {
                buffer[i] = 0;
        }

        // Format the MAC address
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 data->mac_address[0], data->mac_address[1],
                 data->mac_address[2], data->mac_address[3],
                 data->mac_address[4], data->mac_address[5]);

        // Format the BSSID
        char bssid_str[18];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 data->bssid[0], data->bssid[1], data->bssid[2], data->bssid[3],
                 data->bssid[4], data->bssid[5]);

        // Create the final formatted string
        snprintf(buffer, sizeof(buffer),
                 "SSID: %s  BSSID: %s  MAC Address: %s  RSSI: %ld "
                 "dBm  Encryption Type: %u",
                 data->ssid, bssid_str, mac_str, data->rssi,
                 data->encryption_type);

        return buffer;
}
