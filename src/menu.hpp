#pragma once
#include "platform/interface/platform.hpp"
#include "common/configuration.hpp"
#include <optional>

enum class Game : int {
        Unknown = 0,
        MainMenu = 1,
        Clean2048 = 2,
        Minesweeper = 3,
        GameOfLife = 4,
        Settings = 5,
        RandomSeedPicker = 6,
        Snake = 7,
        SnakeDuel = 8,
        WifiApp = 9,
        Sudoku = 10,
        Power = 11,
        Brightness = 12,
        DefaultsSetting = 13,
};

typedef struct GameMenuConfiguration {
        ConfigurationHeader header;
        Game game;
        Color accent_color;
        UserInterfaceRenderingMode rendering_mode;
        bool show_help_text;
} GameMenuConfiguration;

bool is_valid_game(Game game);

Game game_from_string(const char *name);

const char *game_to_string(Game game);

std::optional<UserAction> select_app_and_run(Platform *p);

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_game_menu_defaults_config(Platform *p,
                                  GameMenuConfiguration *configuration);
/**
 * Responsible for getting input from the user when they first turn on the
 * console. This allows for selecting the game and navigating to the setting
 * apps if needed.
 */
std::optional<UserAction>
main_menu_interaction_loop(Platform *p, GameMenuConfiguration *configuration);
