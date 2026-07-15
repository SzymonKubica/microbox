#include <sstream>
#include "geolocation.hpp"
#include "../common/logging.hpp"
#include "../lib/ArduinoJson-v7.4.2.h"

#define TAG "geolocation_service"

std::string quer3 = "https://nominatim.openstreetmap.org/"
                    "search?q=10+Downing+Street,+London&format=json";

const std::string HOST = "nominatim.openstreetmap.org";
const std::string BASE_URL = "https://nominatim.openstreetmap.org/search?q=";
const std::string FORMAT_SUFFIX = "&format=json";

std::optional<std::string> fetch_geolocation(const Platform &p,
                                             std::string query);
Location parse_location(std::string response);
Location GeolocationProvider::search_location(const Platform &p,
                                              std::string query)
{
        auto response = fetch_geolocation(p, query);
        if (!response.has_value())
                return {0.0f, 0.0f};

        Location location = parse_location(response.value());
        LOG_DEBUG(TAG, "Received geolocation: \nLatitude: %f, Longitude: %f",
                  location.latitude, location.longitude);

        return location;
}

std::string assemble_query_url(const std::string &query);
std::optional<std::string> fetch_geolocation(const Platform &p,
                                             std::string query)
{
        ConnectionConfig config{HOST, 443};
        std::string url = assemble_query_url(query);
        LOG_DEBUG(TAG, "Query URL: \n%s", url.c_str());

        auto maybe_response = p.client->get(config, url);

        if (maybe_response.has_value()) {
                LOG_DEBUG(TAG, "Received geolocation response!");
        }
        return maybe_response;
}

/**
 * Assembles the query url as below. Note that the original user query is
 * processed to concatenate separate words using '+' signs.
 * "https://nominatim.openstreetmap.org/"
 * "search?q=10+Downing+Street,+London&format=json";
 */
std::string assemble_query_url(const std::string &query)
{
        std::vector<std::string> parts;

        std::stringstream ss(query);
        std::string token;
        while (getline(ss, token, ' ')) {
                parts.push_back(token);
        }

        std::string url = std::string(BASE_URL);
        for (int i = 0; i < parts.size() - 1; i++) {
                url += parts[i];
                url += "+";
        }
        url += parts[parts.size() - 1];
        url += FORMAT_SUFFIX;
        return url;
}

Location parse_location(std::string response)
{
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, response);
        if (err) {
                LOG_DEBUG(TAG, "Failed to parse response %s", err.c_str());
                return {0, 0};
        }

        float latitude = doc[0]["lat"].as<float>();
        float longitude = doc[0]["lon"].as<float>();
        return {latitude, longitude};
}
