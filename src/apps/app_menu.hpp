#pragma once
#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../menu.hpp"
#include "../application_executor.hpp"
#include <optional>

struct AppMenuConfiguration {
        ConfigurationHeader header;
        Game app;
};

std::optional<UserAction> select_utility_app_and_run(Platform *p);

/**
 * This is the 'game' corresponding to the settings menu. It allows the user
 * to then select the desired settings utility (e.g. wifi, brightness, sleep,
 * ...) and then runs it. The key rationale behind this is that we don't want
 * to clutter the main menu with all of the settings utils.
 */
class UtilityApplicationMenu : public ApplicationExecutor<AppMenuConfiguration>
{
      public:
        UtilityApplicationMenu() {}

        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const AppMenuConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       AppMenuConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
