#include <catch2/catch_test_macros.hpp>
#include "../src/apps/weather.hpp"
#include "../src/platform/emulator/emulator_http_client.hpp"

TEST_CASE("Weather API works", "[weather]")
{
        Platform p{};
        EmulatorHttpClient client{};
        p.client = &client;
        WeatherProvider provider{p};

        WeatherData response = provider.get_weather_data(
            {.latitude = 50.40, .longitude = 18.70}, 5);

        std::cout << "Current weather:" << std::endl;
        std::cout << "Time: " << response.current.timestamp
                  << ", Temperature: " << response.current.temperature
                  << std::endl;
        std::cout << "Hourly weather:" << std::endl;
        for (const auto &hourly : response.hourly) {

                std::cout << "Time: " << hourly.timestamp
                          << ", Temperature: " << hourly.temperature
                          << std::endl;
        }
}
