#pragma once
#include <cstdint>
#include <optional>

/**
 * Encapsulates all information we might want to know about the current network
 * that we are connected to.
 */
typedef struct WifiData {
        /**
         * MAC Address of the current device (e.g. Arduino board running the
         * game console or the PC running the emulator).
         */
        uint8_t mac_address[6];
        /**
         * BSSID of the actual router that we are connected to (its MAC
         * address).
         */
        uint8_t bssid[6];
        /**
         * Human-readable name of the network we are connected to.
         */
        const char *ssid;
        /**
         * Wifi signal strength measured in dBm (the less negative the better).
         */
        long rssi;
        /**
         * Encryption type used by the WiFi network.
         */
        uint8_t encryption_type;
} WifiData;

/**
 * Interface responsible for allowing to connect to Wi-Fi network, and retrieve
 * connection status data (see WifiData above).
 */
class WifiProvider
{
      public:
        /**
         * Returns the WiFi data of the network that we are currently connected
         * to.
         */
        virtual WifiData *get_wifi_data() = 0;
        /**
         * Tries to connect to the network with the given ssid and password.
         * If successful, it will return a pointer to the WifiData struct with
         * the connection details. If failed, the optional will be empty. This
         * is a blocking call that will ont return unitl connection is
         * established or failed.
         */
        virtual std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) = 0;
        /**
         * Tries to connect to the network with the given ssid and password.
         * This is a non-blocking call that will return immediately. The caller
         * is responsbible for checking the connection status later by calling
         * the is_connected() method.
         */
        virtual void connect_to_network_async(const char *ssid,
                                              const char *password) = 0;
        /**
         * Returns true if we are currently connected to a WiFi network,
         */
        virtual bool is_connected() = 0;
};

extern char *get_wifi_data_string(WifiData *data);
