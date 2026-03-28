#pragma once
#include "application_executor.hpp"
#include "common_transitions.hpp"
#include "../common/configuration.hpp"
#include <optional>

typedef struct MinesweeperConfiguration {
        ConfigurationHeader header;
        int mines_num;
} MinesweeperConfiguration;

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_minesweeper_config(Platform *p, MinesweeperConfiguration *game_config,
                           UserInterfaceCustomization *customization);

class Minesweeper : public ApplicationExecutor<MinesweeperConfiguration>
{
      public:
        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const MinesweeperConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       MinesweeperConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;

        Minesweeper() {}
};
