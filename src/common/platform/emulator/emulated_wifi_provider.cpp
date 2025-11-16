#ifdef EMULATOR
#include "../interface/wifi.hpp"
#include "../../logging.hpp"
#include <cstdio>
#include <filesystem>
#include <optional>
#include <regex>
#include <array>
#include <string.h>

class EmulatedWifiProvider : public WifiProvider
{
      private:
        bool connected = false;
        std::string interface;

        /**
         * Executes a supplied shell command and returns its output as a string.
         */
        std::string execute_command(const char *cmd)
        {
                std::array<char, 256> buffer{};
                std::string result;
                FILE *pipe = popen(cmd, "r");
                if (!pipe)
                        return "";
                while (fgets(buffer.data(), buffer.size(), pipe)) {
                        result += buffer.data();
                }
                pclose(pipe);
                return result;
        }

        /**
         * Scans the `/sys/class/net` directory to detect available wireless
         * network interfaces. Note thta the first wireless interface found is
         * returned.
         */
        std::optional<std::string> detect_interface()
        {
                for (auto &entry :
                     std::filesystem::directory_iterator("/sys/class/net")) {
                        std::string iface = entry.path().filename();
                        // Check interface is wireless by checking for wireless
                        // directory, only wireless network interfaces have
                        // this.
                        std::string wireless_path =
                            "/sys/class/net/" + iface + "/wireless";
                        if (std::filesystem::exists(wireless_path)) {
                                return iface;
                        }
                }
                return std::nullopt;
        }

        WifiData *read_wifi_data()
        {
                WifiData *data = new WifiData();

                if (interface.empty()) {
                        auto maybe_interface = detect_interface();
                        if (!maybe_interface.has_value()) {
                                this->connected = false;
                        }
                        interface = maybe_interface.value();
                }

                // Read MAC address (device MAC)
                std::string mac = execute_command(
                    ("cat /sys/class/net/" + interface + "/address").c_str());
                sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &(data->mac_address[0]), &(data->mac_address[1]),
                       &(data->mac_address[2]), &(data->mac_address[3]),
                       &(data->mac_address[4]), &(data->mac_address[5]));

                // Get SSID/BSSID/RSSI via iw
                std::string iw =
                    // execute_command(("iw dev " + interface + "
                    // link").c_str());
                    //  Temporary override to read from a hand-rolled file to
                    //  test iw command output parsing
                    execute_command("cat mock-iw-output");

                if (iw.find("Not connected.") != std::string::npos) {
                        this->connected = false;
                        return nullptr;
                }

                LOG_DEBUG("wifi_emulator", iw.c_str());

                this->connected = true;

                // Parse SSID
                {
                        std::regex re("SSID: (.*)");
                        std::smatch m;
                        if (std::regex_search(iw, m, re)) {
                                data->ssid = strdup(m[1].str().c_str());
                        }
                }

                // Parse BSSID
                {
                        std::regex re("Connected to ([0-9a-fA-F:]+)");
                        std::smatch m;
                        if (std::regex_search(iw, m, re)) {
                                sscanf(m[1].str().c_str(),
                                       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                                       &(data->bssid[0]), &(data->bssid[1]),
                                       &(data->bssid[2]), &(data->bssid[3]),
                                       &(data->bssid[4]), &(data->bssid[5]));
                        }
                }

                // Parse signal strength
                {
                        std::regex re("signal: (-?[0-9]+) dBm");
                        std::smatch m;
                        if (std::regex_search(iw, m, re)) {
                                data->rssi = std::stol(m[1].str());
                        }
                }

                // Encryption: not directly available; set a dummy value
                data->encryption_type = 2;
                LOG_DEBUG("wifi_emulator",
                          "Successfully parsed wifi information.");
                return data;
        }

      public:
        EmulatedWifiProvider() {}

        WifiData *get_wifi_data() override { return read_wifi_data(); }

        std::optional<WifiData *>
        connect_to_network(const char *ssid, const char *password) override
        {
                // Emulator does not connect—only checks if host is on that SSID
                WifiData *data = read_wifi_data();
                if (!connected) {
                        LOG_DEBUG("wifi_emulator",
                                  "Returning empty optional as we are not "
                                  "connected to the Wi-Fi.");
                        return std::nullopt;
                }
                if (strcmp(data->ssid, ssid) == 0) {
                        return data;
                }
                // For now we return connection details even if we are not on
                // the configured network
                return data;
                // return std::nullopt;
        }

        bool is_connected() override
        {
                read_wifi_data();
                return connected;
        }
};
#endif
