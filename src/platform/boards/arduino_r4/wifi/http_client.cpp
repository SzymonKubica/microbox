#if defined(ARDUINO_UNOR4_WIFI)
#include "./http_client.hpp"
#include "../../../../common/logging.hpp"
#include <WiFiS3.h>
#include <Arduino.h>
#include <cstdint>

#define TAG "http_client"

#define TIMEOUT 5000

std::optional<std::string>
ArduinoHttpClient::get(const ConnectionConfig &config, const std::string &url)
{
        WiFiSSLClient client;
        LOG_DEBUG("wifi_client", "Connecting to host...");
        if (!client.connect(config.host.c_str(), (uint16_t)config.port)) {
                LOG_INFO(TAG, "Connection to host failed");
                return std::nullopt;
        }

        LOG_DEBUG("wifi_client", "Connected to host, sending request...");

        // We use HTTP/1.0 to disable chunked encoding, it makes the response
        // easier to parse.
        std::string get_request = "GET " + url + " HTTP/1.0";
        std::string host = "Host: " + config.host;

        client.println(get_request.c_str());
        client.println(host.c_str());
        client.println("Connection: close");
        client.println();

        std::string response;

        unsigned long timeout = millis();
        while (client.connected() || client.available()) {
                while (client.available()) {
                        response += client.read();
                        // reset timeout after each char
                        timeout = millis();
                }

                if (millis() - timeout > TIMEOUT) {
                        LOG_INFO(TAG, "Timeout");
                        break;
                }
                delay(1);
        }

        client.stop();

        // Strip headers
        size_t body_start = response.find("\r\n\r\n");

        if (body_start == std::string::npos) {
                return std::nullopt;
        }

        return response.substr(body_start + 4);
}
#endif
