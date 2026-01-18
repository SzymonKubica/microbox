#pragma once

#include "../common/platform/interface/display.hpp"
#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

typedef struct Game2048Configuration {
        int grid_size;
        int target_max_tile;
        // Indicates whether the game configuration has an ongoing game saved
        // down.
        bool is_game_in_progress;
        // Saved state of the grid if an ongoing game is present.
        // Note that we alloate a 5x5 grid even if the actual grid size is
        // smaller. We need 5x5 as this is the max supported grid size.
        int saved_grid[5][5];
        int saved_grid_size;
        int saved_target_max_tile;
} Game2048Configuration;

class GameState
{
      public:
        int **grid;
        int **old_grid;
        int score;
        int occupied_tiles;
        int grid_size;
        int target_max_tile;

        GameState(int **grid, int **old_grid, int score, int occupied_tiles,
                  int grid_size, int target_max_tile)
            : grid(grid), old_grid(old_grid), score(score),
              occupied_tiles(occupied_tiles), grid_size(grid_size),
              target_max_tile(target_max_tile)
        {
        }
};

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_2048_config(Platform *p, Game2048Configuration *game_config,
                    UserInterfaceCustomization *customization);

GameState *initialize_game_state(int gridSize, int target_max_tile);

void draw(Display *display, GameState *state);
void update_game_grid(Display *display, GameState *gs,
                      UserInterfaceCustomization *customization);

void initialize_randomness_seed(int seed);
bool is_game_over(GameState *gs);
bool is_game_finished(GameState *gs);
void take_turn(GameState *gs, int direction);

class Clean2048 : public GameExecutor
{
      public:
        virtual void
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        Clean2048() {}
};
