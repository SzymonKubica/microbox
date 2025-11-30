#include "../interface/http_client.hpp"

class ArduinoHttpClient : HttpClient {
      public:
        /**
         * Performs a GET request to the specified URL and returns the response
         * body as a string. If the request fails, it returns an empty optional.
         */
        std::optional<std::string> get(const ConnectionConfig &config,
                                               const std::string &url) override;

};
