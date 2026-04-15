#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "application_executor.hpp"

typedef struct BrightnessConfiguration {
        int brightness;
} BrightnessConfiguration;

std::optional<UserAction>
collect_snake_config(Platform *p, BrightnessConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class BrightnessApp : public ApplicationExecutor<BrightnessConfiguration>
{
      public:
        BrightnessApp() {}

        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const BrightnessConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       BrightnessConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;
};
