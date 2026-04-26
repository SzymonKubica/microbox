#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

typedef struct SleepConfiguration {
        bool deep_sleep;
} SleepConfiguration;

std::optional<UserAction>
collect_snake_config(Platform *p, SleepConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class SleepApp : public ApplicationExecutor<SleepConfiguration>
{
      public:
        SleepApp() {}

        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const SleepConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       SleepConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;
};
