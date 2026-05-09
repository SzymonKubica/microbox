#pragma once

#include "../platform/interface/display.hpp"
#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

struct Game2048Configuration {
        ConfigurationHeader header;
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
};

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

GameState *initialize_game_state(int gridSize, int target_max_tile);

void draw(Display *display, GameState *state);
void update_game_grid(const Platform &p, GameState *gs,
                      const UserInterfaceCustomization &customization);

bool is_game_over(GameState *gs);
bool is_game_finished(GameState *gs);
void take_turn(GameState *gs, int direction);

class Clean2048 : public ApplicationExecutor<Game2048Configuration>
{
      public:
        Clean2048() {}

        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const Game2048Configuration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       Game2048Configuration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
};
