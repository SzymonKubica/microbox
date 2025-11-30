#ifndef EMULATOR
#include "./arduino_http_client.hpp"
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

                String response;
                while (client.available()) {
                        response += client.readString();
                }

                Serial.println(response);
                // Extract body (after headers)
                std::string output(response.c_str());
                return output;

        } else {
                Serial.println("Connection to host failed");
                return std::nullopt;
        }
}
#endif
