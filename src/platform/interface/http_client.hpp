#pragma once
#include <string>
#include <optional>

struct ConnectionConfig {
        std::string host;
        int port;
};

class HttpClient
{
      public:
        /**
         * Performs a GET request to the specified URL and returns the response
         * body as a string. If the request fails, it returns an empty optional.
         */
        virtual std::optional<std::string> get(const ConnectionConfig &config,
                                               const std::string &url) = 0;
};
