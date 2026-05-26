#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"
#include <optional>

struct MinesweeperConfiguration {
        ConfigurationHeader header;
        int mines_num;
};

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_minesweeper_config(Platform *p, MinesweeperConfiguration *game_config,
                           UserInterfaceCustomization *customization);

class Minesweeper : public ApplicationExecutor<MinesweeperConfiguration>,
                    public ThumbnailRenderer
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const MinesweeperConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       MinesweeperConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        void render_thumbnail(
            const Platform &platform,
            const UserInterfaceCustomization &customization) override;

        Minesweeper() {}
};
