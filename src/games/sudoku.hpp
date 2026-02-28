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

class SudokuGame : public GameExecutor<SudokuConfiguration>
{
      public:
        UserAction game_loop(Platform *p,
                                     UserInterfaceCustomization *customization,
                                     const SudokuConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       SudokuConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;

        SudokuGame() {}
};
