#include <catch2/catch_test_macros.hpp>
#include "../src/games/2048.cpp"

TEST_CASE("Empty game state is not game over", "[2048]")
{
        GameState *state = initialize_game_state(4, 2048);
        REQUIRE(!is_game_over(*state));
}

TEST_CASE("Full immovable grid is game over", "[2048]")
{
        GameState *state = initialize_game_state(4, 2048);

        std::vector<std::vector<int>> full_grid = {
            {2, 4, 2, 4}, {4, 2, 4, 2}, {2, 4, 2, 4}, {4, 2, 4, 2}};

        for (int i = 0; i < state->grid_size; i++) {
                for (int j = 0; j < state->grid_size; j++) {
                        state->grid[i][j] = full_grid[i][j];
                }
        }
        state->occupied_tiles = 16;
        REQUIRE(is_game_over(*state));
}
