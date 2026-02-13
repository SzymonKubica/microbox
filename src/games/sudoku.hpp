#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

typedef struct SudokuConfiguration {
        int difficulty;
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
