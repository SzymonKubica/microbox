#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"
#include "sudoku_engine.hpp"

#define SUDOKU_GRID_SIZE 9

typedef struct SudokuConfiguration {
        int difficulty;
        bool is_game_in_progress;
        SudokuCell saved_game[SUDOKU_GRID_SIZE][SUDOKU_GRID_SIZE];
} SudokuConfiguration;

std::optional<UserAction>
collect_sudoku_config(Platform *p, SudokuConfiguration *game_config,
                      UserInterfaceCustomization *customization);

class SudokuGame : public GameExecutor
{
      public:
        virtual std::optional<UserAction>
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        SudokuGame() {}
};
