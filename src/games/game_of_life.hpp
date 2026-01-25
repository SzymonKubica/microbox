#pragma once
#include "game_executor.hpp"
#include "common_transitions.hpp"
#include "../common/configuration.hpp"
#include "../common/grid.hpp"
#include <optional>

typedef struct GameOfLifeConfiguration {
        bool prepopulate_grid;
        bool use_toroidal_array;
        /**
         * Simulation steps taken per second
         */
        int simulation_speed;
        /**
         * Controls how many steps the user is allowed to rewind the simulation
         */
        int rewind_buffer_size;
} GameOfLifeConfiguration;


/**
 * Collects the game of life configuration from the user.
 *
 * This is exposed publicly so that the default game configuration saving module
 * can call it, get the new default setttings and save them in the persistent
 * storage.
 *
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_game_of_life_config(Platform *p, GameOfLifeConfiguration *game_config,
                            UserInterfaceCustomization *customization);

class GameOfLife : public GameExecutor
{
      public:
        virtual std::optional<UserAction>
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        GameOfLife() {}
};
