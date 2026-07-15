#include <catch2/catch_test_macros.hpp>
#include "../src/apps/geolocation.hpp"
#include "../src/platform/emulator/emulator_http_client.hpp"

TEST_CASE("Geolocation API works", "[geolocation]")
{
        Platform p{};
        EmulatorHttpClient client{};
        p.client = &client;
        GeolocationProvider provider{p};

        Location response = provider.search_location("Sosnowa 3, Kamieniec");
        std::cout << "Latitude: " << response.latitude
                  << ", Longitude: " << response.longitude << std::endl;
        REQUIRE(response.latitude != 0.0f);
}
