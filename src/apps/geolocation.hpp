#pragma once
#include "../platform/interface/platform.hpp"
#include <string>

struct Location {
        float latitude;
        float longitude;
};

class GeolocationProvider
{
      public:
        Location search_location(const Platform &p, std::string query);
};
