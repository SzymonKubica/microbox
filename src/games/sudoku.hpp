#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"
#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"
#include "sudoku_engine.hpp"

#define SUDOKU_GRID_SIZE 9

struct SudokuConfiguration {
        ConfigurationHeader header;
        int difficulty;
        bool is_game_in_progress;
        SudokuCell saved_game[SUDOKU_GRID_SIZE][SUDOKU_GRID_SIZE];
        Color accent_color;
};

class SudokuGame : public ApplicationExecutor<SudokuConfiguration>,
                   public ThumbnailRenderer
{
      public:
        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const SudokuConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       SudokuConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        void render_thumbnail(
            const Platform &platform,
            const UserInterfaceCustomization &customization) override;

        SudokuGame() {}
};
