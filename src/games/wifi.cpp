#ifndef EMULATOR
#include <WiFiS3.h>
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
            "Enter SSID and password to connect to WiFi network.";

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

        const char *connecting_text = "Connecting to Wi-Fi network...";
        render_wrapped_text(p, customization, connecting_text);
        // We don't have the dummy wifi provider in in the emulator mode yet.
        LOG_INFO(TAG, "Trying to connect to Wi-Fi.");
        std::optional<WifiData *> wifi_data =
            p->wifi_provider->connect_to_network(SECRET_SSID, SECRET_PASS);

        LOG_INFO(TAG,"Received wifi data structure");

        char display_text_buffer[256];
        if (wifi_data.has_value()) {
                WifiData *data = p->wifi_provider->get_wifi_data();
                char *data_string = get_wifi_data_string_single_line(data);
                LOG_DEBUG("%s\n", data_string);
                sprintf(display_text_buffer,
                        "Successfully connected to Wi-Fi!  %s", data_string);
        } else {
                sprintf(display_text_buffer, "Unable to connect to Wi-Fi!");
        }
        render_wrapped_help_text(p, customization, display_text_buffer);
#ifndef EMULATOR
        const char *host = "www.randomnumberapi.com";
        const int port = 80;

        WiFiClient client;
        if (client.connect(host, port)) {
                client.println("GET "
                               "http://www.randomnumberapi.com/api/v1.0/"
                               "random?min=0&max=10000&count=1 HTTP/1.1");
                client.println("Host: www.randomnumberapi.com");
                client.println("Connection: close");
                client.println();

                // Wait for response
                while (client.connected() && !client.available())
                        delay(10);

                String response;
                while (client.available()) {
                        response += client.readString();
                }

                Serial.println(response);
                // Extract body (after headers)
                int bodyStart = response.indexOf("\r\n\r\n");
                if (bodyStart != -1) {
                        String body = response.substring(bodyStart + 4);
                        body.replace("[", "");
                        body.replace("]", "");
                        body.trim(); // remove trailing newlines/spaces
                        unsigned long seed = body.toInt();
                        Serial.print("Random seed from API: ");
                        Serial.println(seed);
                        srand(seed);
                }

                client.stop();
        } else {
                Serial.println("Connection to randomnumberapi.com failed");
        }
#endif
        wait_until_green_pressed(p);

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

/**
 * TODO
 */
Configuration *assemble_wifi_app_configuration(PersistentStorage *storage)
{

        WifiAppConfiguration *initial_config =
            load_initial_wifi_app_config(storage);

        // Initialize the first config option: game gridsize
        auto *ssid = ConfigurationOption::of_strings("SSID", {SECRET_SSID},
                                                     initial_config->ssid);

        auto *password = ConfigurationOption::of_strings(
            "Password", {SECRET_PASS}, initial_config->password);

        auto *connect_on_startup = ConfigurationOption::of_strings(
            "On Boot", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->connect_on_startup));

        free(initial_config);

        auto options = {ssid, password, connect_on_startup};

        return new Configuration("Wi-Fi", options, "Connect");
}

void extract_game_config(WifiAppConfiguration *app_config,
                         Configuration *config)
{
        ConfigurationOption ssid = *config->options[0];
        ConfigurationOption password = *config->options[1];
        ConfigurationOption connect_on_startup = *config->options[2];

        app_config->is_initialized = true;
        sprintf(app_config->ssid, "%s", ssid.get_current_str_value());
        sprintf(app_config->password, "%s", password.get_current_str_value());
        app_config->connect_on_startup = extract_yes_or_no_option(
            connect_on_startup.get_current_str_value());
}

std::optional<UserAction>
collect_wifi_app_config(Platform *p, WifiAppConfiguration *game_config,
                        UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_wifi_app_configuration(p->persistent_storage);

        auto maybe_interrupt_action =
            collect_configuration(p, config, customization);
        if (maybe_interrupt_action) {
                return maybe_interrupt_action;
        }

        extract_game_config(game_config, config);
        return std::nullopt;
}
