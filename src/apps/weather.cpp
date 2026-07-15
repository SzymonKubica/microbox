#include <cstring>
#include <cassert>

#include "../common/logging.hpp"
#include "../lib/ArduinoJson-v7.4.2.h"
#include "../common/maths_utils.hpp"
#include "../menu.hpp"
#include "weather.hpp"
#include "settings.hpp"

#define TAG "random_seed_picker"

WeatherAppConfiguration DEFAULT_WEATHER_APP_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 2},
    .location = "Peniche, PT",
    .action = WeatherAppAction::Fetch,
    .forecast_days = 3};

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);

const char *WeatherApp::get_game_name() const { return "Weather App"; }
const char *WeatherApp::get_help_text() const { return "TODO"; }

UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization);
UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config);
UserAction WeatherApp::app_loop(const Platform &p,
                                const UserInterfaceCustomization &customization,
                                const WeatherAppConfiguration &config) const
{
        switch (config.action) {
        case WeatherAppAction::UpdateLocation:
                return handle_update_location(p, customization);
        case WeatherAppAction::Fetch:
                return handle_fetch(p, customization, config);
        }
        return UserAction::PlayAgain;
}

WeatherAppConfiguration *
load_initial_weather_app_config(PersistentStorage *storage);
UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization)
{
        char *location;
        auto maybe_interrupt = collect_string_input(
            p, customization, "Enter location:", &location);
        if (maybe_interrupt.has_value()) {
                UserAction action = maybe_interrupt.value();
                if (action == UserAction::Exit) {
                        LOG_DEBUG(TAG, "User cancelled modifying the weather "
                                       "query location.");
                }
                return action;
        }

        const auto storage = p.persistent_storage;
        WeatherAppConfiguration *initial_config =
            load_initial_weather_app_config(storage);
        sprintf(initial_config->location, "%s", location);
        int storage_offset = get_settings_storage_offset(Game::WeatherApp);
        storage->put(storage_offset, *initial_config);

        return UserAction::PlayAgain;
}

UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config)
{

        GeolocationProvider geolocation{};
        WeatherProvider weather{};

        Location location = geolocation.search_location(p, config.location);
        WeatherData data =
            weather.get_weather_data(p, location, config.forecast_days);

        char buffer[200];

        auto [time, temp, rain, precipitation] = data.current;
        sprintf(buffer,
                "Time: %s Temperature: %.3f C Rainfall: %.3f mm/h Rain "
                "Probability: %.1f %%",
                time.c_str(), temp, rain, precipitation);
        render_wrapped_help_text(p, customization, buffer);

        auto maybe_interrupt = wait_until_green_pressed(p);
        // Applicable on the emulator only: if the window is closed while
        // waiting for input we need to propagate the `CloseWindow` action back
        // to the top level so that the SFML window can be closed properly.
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt.value();
        }
        return UserAction::PlayAgain;
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
void extract_weather_app_config(WeatherAppConfiguration &initial_config,
                                WeatherAppConfiguration &weather_query_config,
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

        extract_weather_app_config(*initial_config, game_config, *config);
        return std::nullopt;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config)
{
        std::vector<const char *> availabe_actions = {
            WeatherAppActionUtils::to_cstr(WeatherAppAction::Fetch),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::UpdateLocation),
        };

        auto *action = ConfigurationOption::of_strings(
            ">", availabe_actions,
            WeatherAppActionUtils::to_cstr(initial_config.action));

        auto *location = ConfigurationOption::of_strings(
            "Where", {initial_config.location}, initial_config.location);
        auto *forecast_days = ConfigurationOption::of_integers(
            "Days", {1, 2, 3, 4, 5, 6, 7}, initial_config.forecast_days);

        return new Configuration("Weather", {forecast_days, location, action});
}

void extract_weather_app_config(WeatherAppConfiguration &initial_config,
                                WeatherAppConfiguration &weather_query_config,
                                const Configuration &config)
{

        ConfigurationOption *forecast_days = config.options[0];
        // TODO: add support for storing multiple locations
        ConfigurationOption *location = config.options[1];
        ConfigurationOption *action = config.options[2];

        weather_query_config.forecast_days =
            forecast_days->get_curr_int_value();
        int selected_location_idx = location->currently_selected;
        weather_query_config.action =
            WeatherAppActionUtils::from_cstr(action->get_current_str_value())
                .value();
        memcpy(weather_query_config.location, initial_config.location,
               sizeof(char[100]));
}

const std::string HOST = "api.open-meteo.com";
const std::string BASE_URL = "https://api.open-meteo.com//v1/forecast?";

const std::string METRICS_TO_QUERY =
    "current=temperature_2m,rain,precipitation_probability&hourly=temperature_"
    "2m,rain,"
    "precipitation_probability";

std::string assemble_query_url(Location location, int forecast_days);
WeatherData parse_weather_data(std::string response);
WeatherData WeatherProvider::get_weather_data(const Platform &p,
                                              Location location,
                                              int forecast_days)
{
        ConnectionConfig config{HOST, 443};
        std::string query_url = assemble_query_url(location, forecast_days);
        auto maybe_response = p.client->get(config, query_url);

        if (!maybe_response.has_value()) {
                LOG_ERROR(TAG, "Failed to fetch weather data!");
        }

        WeatherData data = parse_weather_data(maybe_response.value());
        return data;
}

std::string assemble_query_url(Location location, int forecast_days)
{
        std::string url = std::string(BASE_URL);
        url += "latitude=" + std::to_string(location.latitude);
        url += "&";
        url += "longitude=" + std::to_string(location.longitude);
        url += "&";
        url += METRICS_TO_QUERY;
        url += "&";
        url += "forecast_days=" + std::to_string(forecast_days);
        return url;
}

WeatherData parse_weather_data(std::string response)
{
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, response);
        if (err) {
                LOG_DEBUG(TAG, "Failed to parse response %s", err.c_str());
                return {};
        }

        const auto &current = doc["current"];
        WeatherDatapoint current_datapoint = {
            .timestamp = current["time"],
            .temperature = current["temperature_2m"].as<float>(),
            .rain = current["rain"].as<float>(),
            .precipitation_probability =
                current["precipitation_probability"].as<float>(),
        };

        const auto &hourly = doc["hourly"];
        int size = hourly["time"].size();
        std::vector<WeatherDatapoint> hourly_datapoints(size,
                                                        WeatherDatapoint{});

        const auto &time = hourly["time"];
        const auto &rain = hourly["rain"];
        const auto &temperature = hourly["temperature_2m"];
        const auto &precipitation_probability =
            hourly["precipitation_probability"];

        for (int i = 0; i < size; i++) {
                hourly_datapoints[i] = {
                    time[i],
                    temperature[i].as<float>(),
                    rain[i].as<float>(),
                    precipitation_probability[i].as<float>(),
                };
        }

        return {current_datapoint, hourly_datapoints};
}
std::optional<WeatherAppAction>
WeatherAppActionUtils::from_cstr(const char *str)
{
        return StrEnum::from_cstr(str, TABLE);
}
