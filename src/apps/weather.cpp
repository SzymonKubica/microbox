#include <cstring>
#include <cassert>

#include "../common/logging.hpp"
#include "../common/maths_utils.hpp"
#include "weather.hpp"
#include "settings.hpp"
#include "../menu.hpp"

#define TAG "random_seed_picker"

WeatherAppConfiguration DEFAULT_WEATHER_APP_CONFIG = {};

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);

const char *WeatherApp::get_game_name() const { return "Weather App"; }
const char *WeatherApp::get_help_text() const
{
        return "Select 'Modify' action and press next (red) to change the seed"
               "Select 'Download' to fetch a new seed from API (wifi "
               "connection "
               "required)."
               "Select 'Spin' to srand";
}

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
        // TODO
        return new Configuration("Weather", {});
}

void extract_weather_app_config(
    WeatherAppConfiguration &random_seed_picker_config,
    const Configuration &config)
{
        // TODO
}
