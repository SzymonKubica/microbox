#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

/**
 * For now we only support the deep sleep mode on esp32. Because of this there
 * is nothing to configure here. Yet we still need the configuration struct
 * to conform to the application interface.
 */
typedef struct SleepConfiguration {
} SleepConfiguration;

std::optional<UserAction>
collect_snake_config(Platform *p, SleepConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class PowerManagementApp : public ApplicationExecutor<SleepConfiguration>
{
      public:
        PowerManagementApp() {}

        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const SleepConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       SleepConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
