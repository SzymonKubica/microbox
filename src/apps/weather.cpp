#include <cstring>
#include <cassert>

#include "../common/logging.hpp"
#include "../common/maths_utils.hpp"
#include "weather.hpp"
#include "settings.hpp"
#include "../menu.hpp"

#define TAG "random_seed_picker"

/*
Design draft

https://nominatim.openstreetmap.org/search?q=3+Ethelburga+Street,+London&format=json

input parameters:
1. query type
2. location description (string provided by the user)
3. action to perform (fetch, update location)


 */

const char *query = "https://api.open-meteo.com/v1/"
                    "forecast?latitude=51.47&longitude=-0.1673&current="
                    "temperature_2m&hourly=temperature_2m";
const char *query2 =
    "https://api.open-meteo.com/v1/"
    "forecast?latitude=52.52&longitude=13.41&hourly=temperature_2m,rain,"
    "precipitation_probability&forecast_days=3";

WeatherAppConfiguration DEFAULT_WEATHER_APP_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 2},
    .location = "London, UK",
    .query_type = WeatherQueryType::Both,
    .action = WeatherAppAction::Fetch,
    .forecast_days = 3};

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);

const char *WeatherApp::get_game_name() const { return "Weather App"; }
const char *WeatherApp::get_help_text() const { return "TODO"; }

UserAction WeatherApp::app_loop(const Platform &p,
                                const UserInterfaceCustomization &customization,
                                const WeatherAppConfiguration &config) const
{
        // TODO
        return UserAction::Exit;
}

WeatherAppConfiguration *
load_initial_weather_app_config(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        WeatherAppConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        WeatherAppConfiguration *output = new WeatherAppConfiguration();

        if (!config.header.validate_against(DEFAULT_WEATHER_APP_CONFIG)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "weather app configuration, using default values.");
                memcpy(output, &DEFAULT_WEATHER_APP_CONFIG,
                       sizeof(WeatherAppConfiguration));
                storage->put(storage_offset, DEFAULT_WEATHER_APP_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(WeatherAppConfiguration));
        }

        return output;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config);
void extract_weather_app_config(
    WeatherAppConfiguration &random_seed_picker_config,
    const Configuration &config);

std::optional<UserAction>
WeatherApp::collect_config(const Platform &p,
                           const UserInterfaceCustomization &customization,
                           WeatherAppConfiguration &game_config) const
{
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        auto config = std::unique_ptr<Configuration>(
            assemble_weather_app_configuration(p, *initial_config));

        auto maybe_interrupt_action =
            collect_configuration(p, *config, customization);
        if (maybe_interrupt_action) {
                return maybe_interrupt_action;
        }

        extract_weather_app_config(game_config, *config);
        return std::nullopt;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config)

{
        std::vector<const char *> available_query_types = {
            WeatherQueryTypeUtils::to_cstr(WeatherQueryType::Current),
            WeatherQueryTypeUtils::to_cstr(WeatherQueryType::Forecast),
            WeatherQueryTypeUtils::to_cstr(WeatherQueryType::Both),
        };

        std::vector<const char *> availabe_actions = {
            WeatherAppActionUtils::to_cstr(WeatherAppAction::Fetch),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::UpdateLocation),
        };

        auto *query_type = ConfigurationOption::of_strings(
            "Query", available_query_types,
            WeatherQueryTypeUtils::to_cstr(initial_config.query_type));
        auto *action = ConfigurationOption::of_strings(
            "Action", availabe_actions,
            WeatherAppActionUtils::to_cstr(initial_config.action));

        auto *location = ConfigurationOption::of_strings(
            "Where", {initial_config.location}, initial_config.location);
        auto *forecast_days = ConfigurationOption::of_integers(
            "Days", {1, 2, 3, 4, 5, 6, 7}, initial_config.forecast_days);

        return new Configuration("Weather",
                                 {query_type, forecast_days, location, action});
}

void extract_weather_app_config(
    WeatherAppConfiguration &random_seed_picker_config,
    const Configuration &config)
{
        // TODO
}

std::optional<WeatherQueryType>
WeatherQueryTypeUtils::from_cstr(const char *str)
{
        return StrEnum::from_cstr(str, TABLE);
}
