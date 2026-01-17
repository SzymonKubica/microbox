#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

#define AVAILABLE_CONFIGURATION_SLOTS 5

typedef enum WifiAppAction {
        // This is currently not available as we can only save one wifi config.
        // TODO: implement this
        AddNew = 0,
        // ShowPassword, future ide
        Modify = 1,
        Connect = 2,
} WifiAppAction;

const char *wifi_app_action_to_string(WifiAppAction action);
WifiAppAction action_from_string(char *string);

/**
 * We need to store the WiFI parameters in fixed-size arrays, otherwise saving
 * it to/from persistent memory only saves down pointers and not the actual
 * strings.
 */
typedef struct WifiCredentials {
        char ssid[100];
        char password[100];
} WifiCredentials;

/**
 * We need to store the WiFI parameters in fixed-size arrays, otherwise saving
 * it to/from persistent memory only saves down pointers and not the actual
 * strings.
 */
typedef struct WifiAppConfiguration {
        /**
         * Required to detect if the wifi app configuration struct has not
         * yet been initialized in the persistent storage.
         */
        bool is_initialized;
        std::size_t curr_config_idx;
        WifiCredentials saved_configurations[AVAILABLE_CONFIGURATION_SLOTS];
        bool connect_on_startup;
        WifiAppAction action;

      public:
        std::vector<WifiCredentials*> get_saved_configs();
        const char *get_currently_selected_ssid() const;
        const char *get_currently_selected_password() const;
} WifiAppConfiguration;

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns empty
 * optional if the configuration was successfully collected. Otherwise, if the
 * user requested exit by pressing the blue button, it returns the Exit action
 * and this needs to be handled by the main game loop.
 */
std::optional<UserAction>
collect_wifi_app_config(Platform *p, WifiAppConfiguration *game_config,
                        UserInterfaceCustomization *customization);

class WifiApp : public GameExecutor
{
      public:
        virtual void
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        WifiApp() {}
};
