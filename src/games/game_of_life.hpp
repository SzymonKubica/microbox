#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"
#include "../common/grid.hpp"
#include <optional>

struct GameOfLifeConfiguration {
        ConfigurationHeader header;
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
};

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

class GameOfLife : public ApplicationExecutor<GameOfLifeConfiguration>
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const GameOfLifeConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       GameOfLifeConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        GameOfLife() {}
};
