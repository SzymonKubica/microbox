#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../common/string_enum.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

#define AVAILABLE_CONFIGURATION_SLOTS 5
#define INITIALIZATION_MAGIC_NUMBER 12345

enum class WifiAppAction {
        AddNew = 0,
        // ShowPassword, future idea
        Modify = 1,
        Connect = 2,
};

namespace WifiActionStr
{
constexpr std::pair<WifiAppAction, const char *> TABLE[] = {
    {WifiAppAction::AddNew, "Add New"},
    {WifiAppAction::Modify, "Modify"},
    {WifiAppAction::Connect, "Connect"}};

constexpr const char *to_cstr(WifiAppAction action)
{
        return StrEnum::to_cstr<WifiAppAction>(action, TABLE);
}
std::optional<WifiAppAction> from_cstr(const char *str);
} // namespace WifiActionStr

/**
 * We need to store the WiFI parameters in fixed-size arrays, otherwise saving
 * it to/from persistent memory only saves down pointers and not the actual
 * strings.
 */
struct WifiCredentials {
        char ssid[100];
        char password[100];
};

/**
 * We need to store the WiFI parameters in fixed-size arrays, otherwise saving
 * it to/from persistent memory only saves down pointers and not the actual
 * strings.
 */

struct WifiAppConfiguration {
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
};

bool wifi_should_connect_at_startup(const Platform &p);
void wifi_connect_async(const Platform &p);

class WifiApp : public ApplicationExecutor<WifiAppConfiguration>
{
      public:
        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const WifiAppConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       WifiAppConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        WifiApp() {}
};
