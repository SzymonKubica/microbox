#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"

enum class WeatherQueryType : uint8_t { Current, Forecast, Both };
enum class WeatherMetric : uint8_t {
        Temperature,
        Rain,
        PrecipitationProbability
};

struct WeatherAppConfiguration {
        ConfigurationHeader header;
        float latitude;
        float longitude;
        WeatherQueryType query_type;
        int forecast_days;
};

const char *query = "https://api.open-meteo.com/v1/"
                    "forecast?latitude=51.47&longitude=-0.1673&current="
                    "temperature_2m&hourly=temperature_2m";

const char *query2 =
    "https://api.open-meteo.com/v1/"
    "forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m,rain,"
    "precipitation_probability&forecast_days=3";

class WeatherApp : public ApplicationExecutor<WeatherAppConfiguration>
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const WeatherAppConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       WeatherAppConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        WeatherApp() {}
};
