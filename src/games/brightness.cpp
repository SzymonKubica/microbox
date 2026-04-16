#include <cassert>
#include "brightness.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"
#include "settings.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "sleep_app"

#if defined(WAVESHARE_2_4_INCH_LCD)
#include "Arduino.h"
#endif

const BrightnessConfiguration DEFAULT_BRIGHTNESS_CONFIGURATION = {
    .brightness = 50,
};

const char *BrightnessApp::get_game_name() { return "BrightnessApp"; };
const char *BrightnessApp::get_help_text()
{
        return "Press right to adjust the brightness.";
};

void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score);

int convert_brightness_to_analog_signal(int brightness)
{
        assert(0 < brightness && brightness <= 100);
        return 255 * brightness / 100;
}

UserAction BrightnessApp::app_loop(Platform *p,
                                   UserInterfaceCustomization *customization,
                                   const BrightnessConfiguration &config)
{

        char *brightness_str;
        auto maybe_interrupt = collect_string_input(
            p, customization, "Enter brightness 0-100%", &brightness_str);
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt.value();
        }
        int brightness = atoi(brightness_str);
        assert(0 < brightness && brightness <= 100);
#if defined(WAVESHARE_2_4_INCH_LCD)
#define DEV_BL_PIN 4
        analogWrite(DEV_BL_PIN,
                    convert_brightness_to_analog_signal(brightness));
#endif
        int storage_offset = get_settings_storage_offset(Game::Brightness);
        auto copy = config;
        copy.brightness = brightness;
        p->persistent_storage->put(storage_offset, copy);
        return UserAction::PlayAgain;
}

BrightnessConfiguration *
load_initial_brightness_config(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::Brightness);

        BrightnessConfiguration config{};
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        BrightnessConfiguration *output = new BrightnessConfiguration();

        if (!config.header.is_valid()) {
                LOG_DEBUG(
                    TAG, "The storage does not contain a valid "
                         "brightness app configuration, using default values.");

                memcpy(output, &DEFAULT_BRIGHTNESS_CONFIGURATION,
                       sizeof(BrightnessConfiguration));
                storage->put(storage_offset, DEFAULT_BRIGHTNESS_CONFIGURATION);
        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(BrightnessConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded brightness app configuration");

        return output;
}

void set_brightness_from_storage(PersistentStorage *storage)
{
        auto config = load_initial_brightness_config(storage);
#if defined(WAVESHARE_2_4_INCH_LCD)
#define DEV_BL_PIN 4
        analogWrite(DEV_BL_PIN,
                    convert_brightness_to_analog_signal(config->brightness));
#endif
}

Configuration *
assemble_brightness_configuration(BrightnessConfiguration *initial_config)
{
        auto *brightness = ConfigurationOption::of_integers(
            "0-100", {initial_config->brightness}, initial_config->brightness);

        auto options = {brightness};

        return new Configuration("Brightness", options);
}

std::optional<UserAction>
BrightnessApp::collect_config(Platform *p,
                              UserInterfaceCustomization *customization,
                              BrightnessConfiguration *game_config)
{
        BrightnessConfiguration *initial_config =
            load_initial_brightness_config(p->persistent_storage);
        Configuration *config =
            assemble_brightness_configuration(initial_config);

        auto maybe_interrupt_action =
            collect_configuration(p, config, customization);
        if (maybe_interrupt_action) {
                delete config;
                delete initial_config;
                return maybe_interrupt_action;
        }

        delete config;
        delete initial_config;
        return std::nullopt;
}
