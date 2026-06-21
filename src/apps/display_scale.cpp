#include <cassert>
#include <cstring>
#include "display_scale.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"
#include "settings.hpp"

#define TAG "sleep_app"

#if defined(WAVESHARE_2_4_INCH_LCD)
#include "Arduino.h"
#endif

#if defined(EMULATOR)
#include "../platform/emulator/sfml_display.hpp"
#endif

const DisplayScaleConfiguration DEFAULT_SCALE_CONFIGURATION = {
    .header = ConfigurationHeader{.magic = CONFIGURATION_MAGIC, .version = 2},
    .scale = 1,
};

const char *DisplayScaleApp::get_game_name() const
{
        return "DisplayScaleApp";
};
const char *DisplayScaleApp::get_help_text() const
{
        return "Select the display scaling factor and confirm.";
};

UserAction
DisplayScaleApp::app_loop(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const DisplayScaleConfiguration &config) const
{
#if defined(EMULATOR)
        LOG_DEBUG(TAG, "Setting display scale to %d", config.scale);
        ((SfmlDisplay *)p.display)->set_scale(config.scale);
#endif
        p.time_provider->delay_ms(150);
        return UserAction::PlayAgain;
}

DisplayScaleConfiguration *
load_initial_display_size_config(const PersistentStorage &storage)
{

        int storage_offset =
            get_settings_storage_offset(Game::DisplaySizeSetting);

        DisplayScaleConfiguration config{};
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage at offset %d",
                  storage_offset);
        storage.get(storage_offset, config);

        DisplayScaleConfiguration *output = new DisplayScaleConfiguration();

        if (!config.header.validate_against(DEFAULT_SCALE_CONFIGURATION)) {
                LOG_DEBUG(
                    TAG,
                    "The storage does not contain a valid "
                    "display scale app configuration, using default values.");

                memcpy(output, &DEFAULT_SCALE_CONFIGURATION,
                       sizeof(DisplayScaleConfiguration));
                storage.put(storage_offset, DEFAULT_SCALE_CONFIGURATION);
        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(DisplayScaleConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded display scale configuration: %d", config.scale);

        return output;
}

Configuration *
assemble_configuration(const DisplayScaleConfiguration &initial_config)
{
        auto *scale = ConfigurationOption::of_integers("Scale", {1, 2},
                                                       initial_config.scale);

        auto options = {scale};

        return new Configuration("Display Scale", options);
}

std::optional<UserAction>
DisplayScaleApp::collect_config(const Platform &p,
                                const UserInterfaceCustomization &customization,
                                DisplayScaleConfiguration &game_config) const
{
        auto initial_config = std::unique_ptr<DisplayScaleConfiguration>(
            load_initial_display_size_config(*p.persistent_storage));
        auto config = std::unique_ptr<Configuration>(
            assemble_configuration(*initial_config));

        auto interrupt = collect_configuration(p, *config, customization);
        if (interrupt)
                return interrupt;

        ConfigurationOption scale_option = *config->options[0];
        game_config.scale = scale_option.get_curr_int_value();

        return std::nullopt;
}
