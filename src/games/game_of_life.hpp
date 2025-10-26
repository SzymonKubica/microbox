#pragma once
#include "game_executor.hpp"
#include "common_transitions.hpp"
#include "../common/configuration.hpp"
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
 * Stores all information required for rendering the finite grid for game of
 * life simulation. Note that this is similar to the struct that we are using
 * for Minesweeper so we might want to abstract away this commonality in the
 * future.
 *
 * Note: this is currently used for the snake grid as well. In the future this
 * should be moved to a shared module. TODO: remove and move to shared module once
 * Snake prototype is done. (Note 2: Minesweeper also uses the same thing).
 */
typedef struct GameOfLifeGridDimensions {
        int rows;
        int cols;
        int top_vertical_margin;
        int left_horizontal_margin;
        int actual_width;
        int actual_height;

        GameOfLifeGridDimensions(int r, int c, int tvm, int lhm, int aw, int ah)
            : rows(r), cols(c), top_vertical_margin(tvm),
              left_horizontal_margin(lhm), actual_width(aw), actual_height(ah)
        {
        }
} GameOfLifeGridDimensions;

GameOfLifeGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius, int game_cell_width);

void draw_game_canvas(Platform *p, GameOfLifeGridDimensions *dimensions,
                      UserInterfaceCustomization *customization);

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
        virtual void
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        GameOfLife() {}
};
