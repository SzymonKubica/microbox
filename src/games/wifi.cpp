#ifndef EMULATOR
#include <WiFiS3.h>
#include "../common/platform/arduino/arduino_http_client.hpp"
#endif
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cstring>
#include "wifi.hpp"

#include "../common/logging.hpp"
#include "../common/platform/arduino/wifi_provider.cpp"
#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../common/platform/arduino/arduino_secrets.hpp"

#include "game_menu.hpp"
#include "common_transitions.hpp"
#include "settings.hpp"

#define TAG "WifiApp"
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

/**
 *  We always render white on black. This is because of the rendering
 *  speed constraints. 2048 required a lot of re-rendering and the UI
 *  needs to be snappy for the proper experience (we don't want the numbers
 *  slowly trickling in when the grid shifts). After testing empirically,
 *  rendering black on white is by far the fastest.
 */
const Color GRID_BG_COLOR = White;
/**
 *  We always render white on black. This is because of the rendering
 *  speed constraints. 2048 required a lot of re-rendering and the UI
 *  needs to be snappy for the proper experience (we don't want the numbers
 *  slowly trickling in when the grid shifts). After testing empirically,
 *  rendering black on white is by far the fastest.
 */
const Color TEXT_COLOR = Black;

WifiAppConfiguration DEFAULT_WIFI_APP_CONFIG;
/**
 * Returns the action that the user wants to take after the game loop is
 * complete. This can either be 'PlayAgain' or they can request to exit or show
 * the help screen. This needs to be appropriately handled by the caller.
 */
UserAction wifi_app_loop(Platform *platform,
                         UserInterfaceCustomization *customization);

void WifiApp::game_loop(Platform *p, UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Select 'Modify' action and press next (red) to enter the new "
            "wifi name and password. Select 'Connect' and press next to "
            "initiate the wifi connection.";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (wifi_app_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_INFO(TAG, "Re-entering the main wifi app loop.");
                        continue;
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_INFO(TAG,
                                 "User requsted help screen for wifi app.");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                }
        }
}

UserAction wifi_app_loop(Platform *p, UserInterfaceCustomization *customization)
{
        WifiAppConfiguration config;

        auto maybe_action = collect_wifi_app_config(p, &config, customization);
        if (maybe_action) {
                return maybe_action.value();
        }

        switch (config.action) {
        case AddNew:
                // TODO: functionality to save multiple networks
        case Modify: {
                LOG_DEBUG(TAG, "Getting user input for SSID...");
                char *ssid =
                    collect_string_input(p, customization, "Enter SSID");
                LOG_DEBUG(TAG, "User entered SSID: %s", ssid);
                LOG_DEBUG(TAG, "Getting user input for password...");
                char *password =
                    collect_string_input(p, customization, "Enter password");
                LOG_DEBUG(TAG, "User entered password: %s", password);

                std::vector<int> offsets = get_settings_storage_offsets();
                int offset = offsets[WifiApp];

                LOG_DEBUG(TAG, "Saving wifi app config at storage offset %d",
                          offset);

                sprintf(config.ssid, "%s", ssid);
                sprintf(config.password, "%s", password);
                p->persistent_storage->put(offset, config);
                break;
        }
        case Connect:
                const char *connecting_text = "Connecting to Wi-Fi network...";
                render_wrapped_text(p, customization, connecting_text);
                // We don't have the dummy wifi provider in in the emulator mode
                // yet.
                LOG_INFO(TAG,
                         "Trying to connect to Wi-Fi using network %s and "
                         "password %s",
                         config.ssid, config.password);
                std::optional<WifiData *> wifi_data =
                    p->wifi_provider->connect_to_network(config.ssid,
                                                         config.password);

                LOG_INFO(TAG, "Received wifi connection data");

                char display_text_buffer[256];
                if (wifi_data.has_value()) {
                        WifiData *data = p->wifi_provider->get_wifi_data();
                        char *data_string =
                            get_wifi_data_string_single_line(data);
                        LOG_DEBUG("%s\n", data_string);
                        sprintf(display_text_buffer,
                                "Successfully connected to Wi-Fi!  %s",
                                data_string);
                } else {
                        sprintf(display_text_buffer,
                                "Unable to connect to Wi-Fi!");
                }
                render_wrapped_help_text(p, customization, display_text_buffer);
                wait_until_green_pressed(p);
                break;
        }

        return UserAction::PlayAgain;
}

WifiAppConfiguration *load_initial_wifi_app_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offsets()[WifiApp];

        WifiAppConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        WifiAppConfiguration *output = new WifiAppConfiguration();

        if (!config.is_initialized) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "wifi app configuration, using default values.");
                // We need to populate the defaults on the fly here as
                // we cannot simply use assignemnt with SECRET_SSID/PASS
                // directly (those are fixed size arrays.)
                sprintf(DEFAULT_WIFI_APP_CONFIG.ssid, "%s", SECRET_SSID);
                sprintf(DEFAULT_WIFI_APP_CONFIG.password, "%s", SECRET_PASS);
                DEFAULT_WIFI_APP_CONFIG.connect_on_startup = false;
                DEFAULT_WIFI_APP_CONFIG.is_initialized = true;
                DEFAULT_WIFI_APP_CONFIG.action = WifiAppAction::Modify;
                memcpy(output, &DEFAULT_WIFI_APP_CONFIG,
                       sizeof(WifiAppConfiguration));
                storage->put(storage_offset, DEFAULT_WIFI_APP_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(WifiAppConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded wifi app configuration");

        return output;
}

Configuration *
assemble_wifi_app_configuration(WifiAppConfiguration *initial_config)
{

        // For now the system only supports saving a single wifi network and
        // password, hence why we use the inital config as the only option
        auto *ssid = ConfigurationOption::of_strings(
            "SSID", {initial_config->ssid}, initial_config->ssid);
        auto *password = ConfigurationOption::of_strings(
            "Password", {initial_config->password}, initial_config->password);

        auto *connect_on_startup = ConfigurationOption::of_strings(
            "On Boot", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->connect_on_startup));

        auto available_actions = {
            wifi_app_action_to_string(WifiAppAction::Connect),
            wifi_app_action_to_string(WifiAppAction::AddNew),
            wifi_app_action_to_string(WifiAppAction::Modify)};

        LOG_DEBUG(TAG, "Current initial config wifi action: %d",
                  initial_config->action);
        auto *app_action = ConfigurationOption::of_strings(
            "Action", available_actions,
            wifi_app_action_to_string(initial_config->action));

        auto options = {ssid, password, connect_on_startup, app_action};

        // TODO: currently this string 'Connect' at the end takes no effect and
        // is ignored since we migrated to the button-driven UI workflow. We
        // need to remove this argument.
        return new Configuration("Wi-Fi", options, "Connect");
}

void extract_game_config(WifiAppConfiguration *app_config,
                         Configuration *config)
{
        ConfigurationOption ssid = *config->options[0];
        ConfigurationOption password = *config->options[1];
        ConfigurationOption connect_on_startup = *config->options[2];
        ConfigurationOption app_action = *config->options[3];

        app_config->is_initialized = true;
        sprintf(app_config->ssid, "%s", ssid.get_current_str_value());
        sprintf(app_config->password, "%s", password.get_current_str_value());
        app_config->connect_on_startup = extract_yes_or_no_option(
            connect_on_startup.get_current_str_value());
        app_config->action =
            action_from_string(app_action.get_current_str_value());
}

std::optional<UserAction>
collect_wifi_app_config(Platform *p, WifiAppConfiguration *game_config,
                        UserInterfaceCustomization *customization)
{
        WifiAppConfiguration *initial_config =
            load_initial_wifi_app_config(p->persistent_storage);
        Configuration *config = assemble_wifi_app_configuration(initial_config);

        auto maybe_interrupt_action =
            collect_configuration(p, config, customization);
        if (maybe_interrupt_action) {
                return maybe_interrupt_action;
        }

        extract_game_config(game_config, config);
        free(initial_config);
        return std::nullopt;
}

const char *wifi_app_action_to_string(WifiAppAction action)
{
        switch (action) {
        case WifiAppAction::AddNew:
                return "Add New";
        case WifiAppAction::Modify:
                return "Modify";
        case WifiAppAction::Connect:
                return "Connect";
        default:
                return "UNKNOWN";
        }
        return "UNKNOWN";
}

WifiAppAction action_from_string(char *name)
{
        if (strcmp(name, wifi_app_action_to_string(WifiAppAction::AddNew)) == 0)
                return WifiAppAction::AddNew;
        if (strcmp(name, wifi_app_action_to_string(WifiAppAction::Modify)) == 0)
                return WifiAppAction::Modify;
        if (strcmp(name, wifi_app_action_to_string(WifiAppAction::Connect)) ==
            0)
                return WifiAppAction::Connect;
        // For now we fallback on this one as a default.
        return WifiAppAction::Connect;
}
