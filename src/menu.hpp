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
        DisplaySizeSetting = 14,
        WeatherApp = 15,
};

namespace GameStr
{
constexpr std::pair<Game, const char *> TABLE[] = {
    {Game::MainMenu, "Main Menu"},
    {Game::Clean2048, "2048"},
    {Game::Minesweeper, "Minesweeper"},
    {Game::GameOfLife, "Game Of Life"},
    {Game::Settings, "Settings"},
    {Game::Snake, "Snake"},
    {Game::SnakeDuel, "Snake Duel"},
    {Game::WifiApp, "Wi-Fi"},
    {Game::Sudoku, "Sudoku"},
    {Game::RandomSeedPicker, "Randomness"},
    {Game::Power, "Power Off"},
    {Game::Brightness, "Brightness"},
    {Game::DefaultsSetting, "Defaults"},
    {Game::DisplaySizeSetting, "Display Size"},
    {Game::WeatherApp, "Weather"},
};
constexpr const char *to_cstr(Game type)
{
        for (auto [t, str] : TABLE) {
                if (t == type)
                        return str;
        }
        return "UNKNOWN";
}
std::optional<Game> from_cstr(const char *str);
} // namespace GameStr

struct GameMenuConfiguration {
        ConfigurationHeader header;
        Game game;
        Color accent_color;
        UserInterfaceRenderingMode rendering_mode;
        bool show_help_text;
};

std::optional<UserAction> select_app_and_run(const Platform &p);

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction>
collect_game_menu_defaults_config(const Platform &p,
                                  GameMenuConfiguration &configuration);
/**
 * Responsible for getting input from the user when they first turn on the
 * console. This allows for selecting the game and navigating to the setting
 * apps if needed.
 */
std::optional<UserAction>
main_menu_interaction_loop(const Platform &p,
                           GameMenuConfiguration &configuration);
