#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "application_executor.hpp"

#define AVAILABLE_CONFIGURATION_SLOTS 5
#define INITIALIZATION_MAGIC_NUMBER 12345

typedef enum WifiAppAction {
        AddNew = 0,
        // ShowPassword, future idea
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
        ConfigurationHeader header;
        /**
         * Required to detect if the wifi app configuration struct has not
         * yet been initialized in the persistent storage. If this does not
         * match the initialization magic number, we assume that
         */
        int intialization_magic_number;
        std::size_t curr_config_idx;
        int occupied_config_slots;
        WifiCredentials saved_configurations[AVAILABLE_CONFIGURATION_SLOTS];
        bool connect_on_startup;
        WifiAppAction action;

      public:
        std::vector<WifiCredentials *> get_saved_configs();
        const char *get_currently_selected_ssid() const;
        const char *get_currently_selected_password() const;
        bool is_initialized() const;

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

class WifiApp : public ApplicationExecutor<WifiAppConfiguration>
{
      public:
        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const WifiAppConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       WifiAppConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;

        WifiApp() {}
};
