#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

struct PongConfiguration {
        ConfigurationHeader header;
        int initial_speed;
};

class Pong : public ApplicationExecutor<PongConfiguration>
{
      public:
        Pong() {}

        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const PongConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       PongConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
