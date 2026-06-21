#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

struct DisplayScaleConfiguration {
        ConfigurationHeader header;
        int scale = 1;
};

class DisplayScaleApp : public ApplicationExecutor<DisplayScaleConfiguration>
{
      public:
        DisplayScaleApp() {}

        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const DisplayScaleConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       DisplayScaleConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
