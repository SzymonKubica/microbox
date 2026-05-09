#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

struct BrightnessConfiguration {
        ConfigurationHeader header;
        int brightness;
};

void set_brightness_from_storage(const PersistentStorage &storage);

class BrightnessApp : public ApplicationExecutor<BrightnessConfiguration>
{
      public:
        BrightnessApp() {}

        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const BrightnessConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       BrightnessConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
