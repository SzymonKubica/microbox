#ifdef EMULATOR
#include "./emulator_http_client.hpp"
#include <iostream>
#include <curl/curl.h>

// Callback to store response data
size_t write_callback(void *contents, size_t size, size_t nmemb, std::string *s)
{
        size_t total_size = size * nmemb;
        s->append((char *)contents, total_size);
        return total_size;
}

std::optional<std::string>
EmulatorHttpClient::get(const ConnectionConfig &config, const std::string &url)
{
        CURL *curl;
        CURLcode res;
        std::string response;

        curl = curl_easy_init();
        if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                        std::cerr << "curl error: " << curl_easy_strerror(res)
                                  << std::endl;
                } else {
                        std::cout << response << std::endl;
                }

                curl_easy_cleanup(curl);
        } else {
                return std::nullopt;
        }

        return response;
}

#endif
