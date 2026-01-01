#ifndef EMULATOR
#include "./arduino_http_client.hpp"
#include "../../logging.hpp"
#include <WiFiS3.h>
#include <cstdint>

std::optional<std::string>
ArduinoHttpClient::get(const ConnectionConfig &config, const std::string &url)
{
        WiFiClient client;
        if (client.connect(config.host.c_str(), (uint16_t)config.port)) {
                std::string get_request = "GET " + url + " HTTP/1.1";
                std::string host = "Host: " + config.host;
                client.println(get_request.c_str());
                client.println(host.c_str());
                client.println("Connection: close");
                client.println();

                // Wait for response
                while (client.connected() && !client.available())
                        delay(4);

                std::string response;
                while (client.available()) {
                        response += std::string(client.readString().c_str());
                }

                LOG_DEBUG("wifi_client", response.c_str());

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
}
#endif
