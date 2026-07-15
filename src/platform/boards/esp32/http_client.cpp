#include "../../interface/wifi.hpp"
#if defined(ARDUINO_ARCH_ESP32)
#include "./http_client.hpp"
#include "../../../common/logging.hpp"
#include <WiFi.h>
#include <cstdint>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define TAG "http_client"

std::optional<std::string> Esp32HttpClient::get(const ConnectionConfig &config,
                                                const std::string &url)
{
        WiFiClientSecure client;
        HTTPClient https;
        client.setInsecure();
        LOG_DEBUG("wifi_client", "Connecting to host...");
        if (https.begin(client, url.c_str())) {
                https.addHeader("User-Agent", "MicroBox/1.0 (ESP32)");
                LOG_DEBUG("wifi_client",
                          "Connected to host, sending request...");

                int httpCode = https.GET();

                if (httpCode > 0) {
                        Serial.printf("HTTP code: %d\n", httpCode);
                        String payload = https.getString();
                        std::string output = std::string(payload.c_str());
                        return output;
                }
        } else {
                LOG_INFO(TAG, "Connection to host failed");
                return std::nullopt;
        }
        return std::nullopt;
}
#endif
