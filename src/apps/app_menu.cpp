#include "app_menu.hpp"

#include <cstring>
#include <optional>
#include <stdlib.h>

#include "../common/configuration.hpp"
#include "../application_executor.hpp"
#include "../common/logging.hpp"
#include "settings.hpp"
#include "sleep.hpp"
#include "wifi.hpp"
#include "brightness.hpp"
#include "settings.hpp"
#include "random_seed_picker.hpp"

#define TAG "application_menu"

AppMenuConfiguration DEFAULT_APP_MENU_CONFIGURATION = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1},
    .app = Game::DefaultsSetting,
};

AppMenuConfiguration *
load_initial_utility_menu_configuration(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::Settings);

        AppMenuConfiguration configuration{};

        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, configuration);

        AppMenuConfiguration *output = new AppMenuConfiguration();

        if (!configuration.header.validate_against(
                DEFAULT_APP_MENU_CONFIGURATION)) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "app menu configuration, using default values.");
                memcpy(output, &DEFAULT_APP_MENU_CONFIGURATION,
                       sizeof(AppMenuConfiguration));
                storage->put(storage_offset, DEFAULT_APP_MENU_CONFIGURATION);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &configuration, sizeof(AppMenuConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded menu configuration: game=%d, ", output->app);

        return output;
}

Configuration *
assemble_utility_selector_configuration(Platform *p,
                                        AppMenuConfiguration *initial_config);

void extract_app_selection(AppMenuConfiguration *menu_configuration,
                           Configuration *config);

UserAction
UtilityApplicationMenu::app_loop(Platform *p,
                                 UserInterfaceCustomization *customization,
                                 const AppMenuConfiguration &config)
{
        switch (config.app) {
        case Game::WifiApp:
                execute_app(new WifiApp(), p, customization).value();
                break;
        case Game::RandomSeedPicker:
                execute_app(new RandomSeedPicker(), p, customization).value();
                break;
        case Game::Sleep:
                execute_app(new SleepApp(), p, customization).value();
                break;
        case Game::Brightness:
                execute_app(new BrightnessApp(), p, customization).value();
                break;
        case Game::DefaultsSetting:
                execute_app(new Settings(), p, customization).value();
                break;
        default:
                LOG_DEBUG(TAG, "Unsupported application selected, exiting...");
                return UserAction::Exit;
        }
        // We always play again after the app loop here. This is to ensure that
        // pressing back on one of the setting apps doesn't immediately take us
        // back to the main menu.
        return UserAction::PlayAgain;
}

Configuration *
assemble_utility_selector_configuration(Platform *p,
                                        AppMenuConfiguration *initial_config)
{

        std::vector<const char *> available_apps = {
            game_to_string(Game::Brightness),
            game_to_string(Game::RandomSeedPicker),
            game_to_string(Game::DefaultsSetting),
        };

        if (p->capabilities.has_wifi)
                available_apps.push_back(game_to_string(Game::WifiApp));
        if (p->capabilities.can_sleep)
                available_apps.push_back(game_to_string(Game::Sleep));

        auto *app = ConfigurationOption::of_strings(
            "Configure", available_apps, game_to_string(initial_config->app));

        auto options = {app};

        return new Configuration("Settings", options);
}

void extract_app_selection(AppMenuConfiguration *menu_configuration,
                           Configuration *config)
{
        ConfigurationOption *app_option = config->options[0];

        menu_configuration->app =
            game_from_string(app_option->get_current_str_value());
}

std::optional<UserAction> UtilityApplicationMenu::collect_config(
    Platform *p, UserInterfaceCustomization *customization,
    AppMenuConfiguration *game_config)
{
        AppMenuConfiguration *initial_config =
            load_initial_utility_menu_configuration(p->persistent_storage);

        LOG_DEBUG(TAG, "Initial configuration was loaded.");

        Configuration *config =
            assemble_utility_selector_configuration(p, initial_config);

        auto maybe_interrupt =
            collect_configuration(p, config, customization, true, false);

        if (maybe_interrupt) {
                delete config;
                delete initial_config;
                return maybe_interrupt;
        }
        extract_app_selection(game_config, config);

        delete config;
        delete initial_config;
        return std::nullopt;
}

const char *UtilityApplicationMenu::get_game_name() { return "Utility Menu"; }
const char *UtilityApplicationMenu::get_help_text()
{
        return "Move joystick left/right to select the utility app. Press "
               "'right' to enter the selected app.";
}
