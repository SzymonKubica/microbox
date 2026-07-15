#pragma once
#include "../platform/interface/platform.hpp"
#include <string>

struct Location {
        float latitude;
        float longitude;
};

class GeolocationProvider
{
        const Platform &platform;

      public:
        GeolocationProvider(const Platform &platform) : platform(platform) {}
        Location search_location(std::string query);
};
