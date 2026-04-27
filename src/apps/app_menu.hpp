#pragma once
#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../menu.hpp"
#include "../application_executor.hpp"
#include <optional>

typedef struct AppMenuConfiguration {
        ConfigurationHeader header;
        Game app;
} AppMenuConfiguration;

std::optional<UserAction> select_utility_app_and_run(Platform *p);

class UtilityApplicationMenu : public ApplicationExecutor<AppMenuConfiguration>
{
      public:
        UtilityApplicationMenu() {}

        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const AppMenuConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       AppMenuConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;
};

