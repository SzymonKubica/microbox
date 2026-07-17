#include <chrono>
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
    .curr_config_idx = 0,
    .occupied_config_slots = 3,
    .locations = {"Peniche, PT", "London, UK", "Sosnowa 3, Kamieniec, PL"},
    .action = WeatherAppAction::Fetch,
    .forecast_days = 5};

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);

const char *WeatherApp::get_game_name() const { return "Weather App"; }
const char *WeatherApp::get_help_text() const { return "TODO"; }

UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       const WeatherAppConfiguration &config);
UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config);
UserAction handle_add_new(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const WeatherAppConfiguration &config);
UserAction WeatherApp::app_loop(const Platform &p,
                                const UserInterfaceCustomization &customization,
                                const WeatherAppConfiguration &config) const
{
        switch (config.action) {
        case WeatherAppAction::UpdateLocation:
                return handle_update_location(p, customization, config);
        case WeatherAppAction::Fetch:
                return handle_fetch(p, customization, config);
        case WeatherAppAction::AddNew:
                return handle_add_new(p, customization, config);
        }
        return UserAction::PlayAgain;
}

WeatherAppConfiguration *
load_initial_weather_app_config(PersistentStorage *storage);
UserAction
handle_update_location(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       const WeatherAppConfiguration &config)
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

        WeatherAppConfiguration copy = config;
        // We need to ensure that the header version is not outdated
        copy.header.version = DEFAULT_WEATHER_APP_CONFIG.header.version;
        const auto storage = p.persistent_storage;
        sprintf(copy.locations[config.curr_config_idx], "%s", location);
        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        // Before saving the updated values we need to restore the default
        // action to avoid overwriting it with the 'update location' action that
        // we have received in the current `config`
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        copy.action = initial_config->action;

        storage->put(storage_offset, copy);

        return UserAction::PlayAgain;
}

UserAction handle_fetch(const Platform &p,
                        const UserInterfaceCustomization &customization,
                        const WeatherAppConfiguration &config)
{

        GeolocationProvider geolocation{p};
        WeatherProvider weather{p};

        std::string location_description =
            std::string(config.locations[config.curr_config_idx]);
        LOG_DEBUG(TAG, "Fetching weather data for location: %s",
                  location_description.c_str());

        Location location = geolocation.search_location(location_description);
        auto data = weather.get_weather_data(location, config.forecast_days);

        auto [time, temp, rain, precipitation] = data.current;

        char buffer[200];
        sprintf(buffer,
                "Time: %s "
                "Precipitation: %.1f %% "
                "Temperature: %.1f Cel. "
                "Forecast %d days:",
                time.c_str(), precipitation, temp, config.forecast_days);
        render_wrapped_text(p, customization, buffer);

        int y_start =
            p.display->get_font_configuration().font_dimensions.height * 7;

        std::vector<float> temperatures;
        std::vector<std::string> timestamps;
        for (const auto &datapoint : data.hourly) {
                temperatures.push_back(datapoint.temperature);
                timestamps.push_back(datapoint.timestamp);
        }
        std::vector<std::string> labels;
        std::istringstream iss{data.current.timestamp};
        std::chrono::year_month_day ymd;
        iss >> std::chrono::parse("%FT%R", ymd);
        std::chrono::sys_days sd{ymd};
        for (int i = 0; i < config.forecast_days; i++) {
                char buffer[6];
                std::chrono::year_month_day curr{sd};
                sprintf(buffer, "%d-%d", unsigned(curr.month()),
                        unsigned(curr.day()));
                labels.push_back(std::string(buffer));
                sd += std::chrono::days{1};
        }

        std::string s = data.current.timestamp;
        int curr_hour = std::stoi(s.substr(11, 2));

        int current_idx = timestamps.size();
        for (int i = 0; i < timestamps.size(); i++) {
                std::string s = timestamps[i];
                int hour = std::stoi(s.substr(11, 2));

                if (curr_hour == hour) {
                        current_idx = i;
                        break;
                }
        }

        render_bar_graph(p, customization, y_start, labels, temperatures,
                         current_idx);

        auto maybe_interrupt = wait_until_green_pressed(p);
        // Applicable on the emulator only: if the window is closed while
        // waiting for input we need to propagate the `CloseWindow` action back
        // to the top level so that the SFML window can be closed properly.
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt.value();
        }
        return UserAction::PlayAgain;
}

UserAction handle_add_new(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const WeatherAppConfiguration &config)
{
        if (config.occupied_config_slots >= AVAILABLE_CONFIGURATION_SLOTS) {
                render_wrapped_text(
                    p, customization,
                    "You have reached the maximum number of saved "
                    "locations. Please overwrite an existing location to add a "
                    "new one.");
                auto maybe_interrupt = wait_until_green_pressed(p);
                if (maybe_interrupt.has_value()) {
                        return maybe_interrupt.value();
                }
                return UserAction::PlayAgain;
        }

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

        WeatherAppConfiguration copy = config;
        // We need to ensure that the header version is not outdated
        copy.header.version = DEFAULT_WEATHER_APP_CONFIG.header.version;
        const auto storage = p.persistent_storage;
        int new_config_idx = config.occupied_config_slots;
        copy.occupied_config_slots++;
        sprintf(copy.locations[new_config_idx], "%s", location);
        int storage_offset = get_settings_storage_offset(Game::WeatherApp);

        // Before saving the updated values we need to restore the default
        // action to avoid overwriting it with the 'add new' action that
        // we have received in the current `config`
        auto initial_config = std::unique_ptr<WeatherAppConfiguration>(
            load_initial_weather_app_config(p.persistent_storage));
        copy.action = initial_config->action;

        storage->put(storage_offset, copy);

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

/**
 * Returns the saved configurations as a vector. This is used to allow
 * for more convenient processing even though the actual representation
 * needs to be a simple array so that we can safely serialize it to raw
 * bytes in EEPROM.
 */
std::vector<const char *> WeatherAppConfiguration::get_saved_locations() const
{
        std::vector<const char *> output;
        for (std::size_t i = 0; i < occupied_config_slots; i++) {
                output.push_back(this->locations[i]);
        }
        return output;
}

Configuration *assemble_weather_app_configuration(
    const Platform &p, const WeatherAppConfiguration &initial_config)
{
        std::vector<const char *> availabe_actions = {
            WeatherAppActionUtils::to_cstr(WeatherAppAction::Fetch),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::UpdateLocation),
            WeatherAppActionUtils::to_cstr(WeatherAppAction::AddNew),
        };

        auto *action = ConfigurationOption::of_strings(
            ">", availabe_actions,
            WeatherAppActionUtils::to_cstr(initial_config.action));

        int occupied_slots = initial_config.occupied_config_slots;
        std::vector<const char *> locations =
            initial_config.get_saved_locations();

        auto *location = ConfigurationOption::of_strings(
            "Where", locations, locations[initial_config.curr_config_idx]);
        auto *forecast_days = ConfigurationOption::of_integers(
            "Days", {1, 2, 3, 4, 5}, initial_config.forecast_days);

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
        weather_query_config.curr_config_idx = selected_location_idx;
        weather_query_config.occupied_config_slots =
            initial_config.occupied_config_slots;
        memcpy(weather_query_config.locations, initial_config.locations,
               sizeof(char[AVAILABLE_CONFIGURATION_SLOTS][100]));
}

const std::string HOST = "api.open-meteo.com";
const std::string BASE_URL = "https://api.open-meteo.com//v1/forecast?";

const std::string METRICS_TO_QUERY =
    "current=temperature_2m,rain,precipitation_probability&hourly=temperature_"
    "2m,rain,"
    "precipitation_probability&timezone=auto";

std::string assemble_query_url(Location location, int forecast_days);
WeatherData parse_weather_data(std::string response);
WeatherData WeatherProvider::get_weather_data(Location location,
                                              int forecast_days)
{
        ConnectionConfig config{HOST, 443};
        std::string query_url = assemble_query_url(location, forecast_days);
        auto maybe_response = platform.client->get(config, query_url);

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
        const auto &time = hourly["time"];
        const auto &rain = hourly["rain"];
        const auto &temperature = hourly["temperature_2m"];
        const auto &precipitation_probability =
            hourly["precipitation_probability"];

        int size = hourly["time"].size();
        auto empty = WeatherDatapoint{};
        std::vector<WeatherDatapoint> hourly_datapoints(size, empty);
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

void WeatherApp::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{
        const auto &display = *platform.display;
        clear_half_display_and_render_subtitle(platform, customization,
                                               "Weather");

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();
        // [BEGIN lopaka generated]
        // ellipse 22
        tft.fillEllipse(142, 119, 16, 16, 0xFF47);
        // ellipse 18
        tft.fillEllipse(158, 135, 14, 16, 0xFFFF);
        // ellipse 18 copy 1
        tft.fillEllipse(181, 144, 13, 13, 0xFFFF);
        // ellipse 18 copy 2
        tft.fillEllipse(138, 146, 11, 11, 0xFFFF);
        // rect 21
        tft.fillRect(139, 146, 43, 12, 0xFFFF);
        // ellipse 18 copy 3
        tft.fillEllipse(163, 149, 8, 8, 0xEF7D);
        // ellipse 18 copy 4
        tft.fillEllipse(172, 155, 8, 8, 0xEF7D);
        // ellipse 18 copy 5
        tft.fillEllipse(154, 155, 8, 8, 0xEF7D);
        // rect 26
        tft.fillRect(155, 153, 15, 11, 0xEF7D);
        // line 27
        tft.drawLine(157, 164, 157, 170, 0x24BE);
        // line 27 copy 1
        tft.drawLine(162, 164, 162, 170, 0x24BE);
        // line 27 copy 2
        tft.drawLine(167, 164, 167, 170, 0x24BE);
        // ellipse 18 copy 6
        tft.fillEllipse(186, 111, 8, 8, 0xEF7D);
        // ellipse 18 copy 7
        tft.fillEllipse(177, 112, 9, 9, 0xEF7D);
        // ellipse 18 copy 8
        tft.fillEllipse(186, 118, 8, 8, 0xEF7D);
        // line 27 copy 3
        tft.drawLine(172, 164, 172, 170, 0x24BE);
        // line 27 copy 4
        tft.drawLine(152, 164, 152, 170, 0x24BE);
        // line 27 copy 5
        tft.drawLine(177, 162, 177, 168, 0x24BE);
        // line 27 copy 6
        tft.drawLine(182, 158, 182, 164, 0x24BE);
        // line 27 copy 7
        tft.drawLine(187, 157, 187, 163, 0x24BE);
        // line 27 copy 8
        tft.drawLine(147, 160, 147, 166, 0x24BE);
        // line 27 copy 9
        tft.drawLine(142, 158, 142, 164, 0x24BE);
        // line 27 copy 10
        tft.drawLine(137, 158, 137, 164, 0x24BE);
        // [END lopaka generated]
}
