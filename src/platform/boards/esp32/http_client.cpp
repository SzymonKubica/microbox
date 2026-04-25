#include "../../interface/wifi.hpp"
#if defined(ARDUINO_ARCH_ESP32)
#include "./http_client.hpp"
#include "../../../common/logging.hpp"
#include <WiFi.h>
#include <cstdint>

std::optional<std::string> Esp32HttpClient::get(const ConnectionConfig &config,
                                                const std::string &url)
{
        WiFiClient client;
        LOG_DEBUG("wifi_client", "Connecting to host...");
        if (client.connect(config.host.c_str(), (uint16_t)config.port)) {
                LOG_DEBUG("wifi_client",
                          "Connected to host, sending request...");
                std::string get_request = "GET " + url + " HTTP/1.1";
                std::string host = "Host: " + config.host;
                client.println(get_request.c_str());
                client.println(host.c_str());
                client.println("Connection: close");
                client.println();

                std::string response;
                char buf[64];
                unsigned long start = millis();
                while (client.connected() || client.available()) {
                        if (client.available()) {
                                int len = client.readBytes(buf, sizeof(buf));
                                response.append(buf, len);

                                start =
                                    millis(); // reset timeout after each chunk
                        } else {
                                // No data, wait a tiny bit
                                delay(1);
                        }

                        // Timeout guard
                        if (millis() - start > 3000) { // 3s without data
                                break;
                        }
                }

                int body_start = response.find("\r\n\r\n");
                if (body_start != -1) {
                        std::string body = response.substr(body_start + 4);
                        return body;
                } else {
                        return std::nullopt;
                }

        } else {
                Serial.println("Connection to host failed");
                return std::nullopt;
        }
        return std::nullopt;
}
#endif
