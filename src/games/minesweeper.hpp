#pragma once
#include "game_executor.hpp"
#include "common_transitions.hpp"
#include "../common/configuration.hpp"
#include <optional>

typedef struct MinesweeperConfiguration {
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

class Minesweeper : public GameExecutor
{
      public:
        virtual std::optional<UserAction>
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        Minesweeper() {}
};
