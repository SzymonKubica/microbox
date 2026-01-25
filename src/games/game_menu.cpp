#include <optional>
#include <stdlib.h>
#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/platform/interface/color.hpp"
#include "../common/constants.hpp"
#include "2048.hpp"
#include "game_executor.hpp"
#include "minesweeper.hpp"
#include "settings.hpp"
#include "wifi.hpp"
#include "game_of_life.hpp"
#include "random_seed_picker.hpp"
#include "snake.hpp"
#include "snake_duel.hpp"

#define TAG "game_menu"

GameMenuConfiguration DEFAULT_MENU_CONFIGURATION = {
    .game = Game::GameOfLife,
    .accent_color = DarkBlue,
    .rendering_mode = Minimalistic,
    .show_help_text = true,
};

const char *game_to_string(Game game);

GameMenuConfiguration *
load_initial_menu_configuration(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::MainMenu);

        GameMenuConfiguration configuration = {.game = Game::Unknown,
                                               .accent_color = DarkBlue,
                                               .rendering_mode = Minimalistic,
                                               .show_help_text = true};

        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, configuration);

        GameMenuConfiguration *output = new GameMenuConfiguration();

        if (!is_valid_game(configuration.game)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "game menu configuration, using default values.");
                memcpy(output, &DEFAULT_MENU_CONFIGURATION,
                       sizeof(GameMenuConfiguration));
                storage->put(storage_offset, DEFAULT_MENU_CONFIGURATION);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &configuration, sizeof(GameMenuConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded menu configuration: game=%d, "
                  "accent_color=%d, show_help_text=%d",
                  output->game, output->accent_color, output->show_help_text);

        return output;
}

Configuration *
assemble_menu_selection_configuration(GameMenuConfiguration *initial_config)
{

        auto *game = ConfigurationOption::of_strings(
            "Game",
            {game_to_string(Game::Minesweeper), game_to_string(Game::Clean2048),
             game_to_string(Game::GameOfLife), game_to_string(Game::Snake),
             game_to_string(Game::SnakeDuel),
        // Disable the WiFi app on the Uno R4 Minima
#if defined(ARDUINO_UNOR4_WIFI) || defined(EMULATOR)
             game_to_string(Game::WifiApp),
#endif
             game_to_string(Game::Settings),
             game_to_string(Game::RandomSeedPicker)},
            game_to_string(initial_config->game));

        auto *accent_color = ConfigurationOption::of_colors(
            "Color", AVAILABLE_COLORS, initial_config->accent_color);

        auto available_modes = {
            rendering_mode_to_str(UserInterfaceRenderingMode::Minimalistic),
            rendering_mode_to_str(UserInterfaceRenderingMode::Detailed)};

        auto *rendering_mode = ConfigurationOption::of_strings(
            "UI", available_modes,
            rendering_mode_to_str(initial_config->rendering_mode));

        auto *show_help_text = ConfigurationOption::of_strings(
            "Hints", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->show_help_text));

        auto options = {game, accent_color, rendering_mode, show_help_text};

        return new Configuration("MicroBox", options);
}

void extract_game_config(GameMenuConfiguration *menu_configuration,
                         Configuration *config)
{
        ConfigurationOption *game_option = config->options[0];
        ConfigurationOption *accent_color = config->options[1];
        ConfigurationOption *rendering_mode = config->options[2];
        ConfigurationOption *show_help_text = config->options[3];

        menu_configuration->game =
            game_from_string(game_option->get_current_str_value());
        menu_configuration->accent_color =
            accent_color->get_current_color_value();
        menu_configuration->rendering_mode =
            rendering_mode_from_str(rendering_mode->get_current_str_value());
        menu_configuration->show_help_text =
            extract_yes_or_no_option(show_help_text->get_current_str_value());
}

std::optional<UserAction> select_game(Platform *p)
{
        GameMenuConfiguration config;

        auto maybe_interrupt = collect_game_menu_config(p, &config);

        // This customization might not be initialized properly if the user
        // requests help message. The current version of the help text rendering
        // does not depend on it but this might become problematic in the
        // future.
        UserInterfaceCustomization customization = {
            config.accent_color, config.rendering_mode, config.show_help_text};

        const char *help_text =
            "Move joystick up/down to switch between menu options. Move "
            "joystick left/right or press green to change the value of the "
            "current option. Press green or move joystick left on the last "
            "cell to start the game.";

        if (maybe_interrupt.has_value() &&
            maybe_interrupt.value() == UserAction::ShowHelp) {
                render_wrapped_help_text(p, &customization, help_text);
                return wait_until_green_pressed(p);
        }

        // This is needed to handle the 'close window' action.
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt;
        }

        LOG_INFO(TAG, "User selected game: %s.", game_to_string(config.game));

        GameExecutor *executor = [&]() -> GameExecutor * {
                switch (config.game) {
                case Game::Unknown:
                case Game::Clean2048:
                        return new class Clean2048();
                case Game::Minesweeper:
                        return new class Minesweeper();
                case Game::GameOfLife:
                        return new class GameOfLife();
                case Game::Settings:
                        return new class Settings();
                case Game::Snake:
                        return new class SnakeGame();
                case Game::SnakeDuel:
                        return new class SnakeDuel();
                case Game::WifiApp:
                        return new class WifiApp();
                case Game::RandomSeedPicker:
                        return new class RandomSeedPicker();
                default:
                        return NULL;
                }
        }();

        if (!executor) {
                LOG_DEBUG(TAG, "Selected game: %d. Game not implemented yet.",
                          config.game);
                return std::nullopt;
        }

        auto maybe_action = executor->game_loop(p, &customization);

        if (maybe_action.has_value() &&
            maybe_action.value() == UserAction::CloseWindow) {
                delete executor;
                return UserAction::CloseWindow;
        }
        delete executor;
        return std::nullopt;
}

std::optional<UserAction>
collect_game_menu_config(Platform *p, GameMenuConfiguration *configuration)
{

        GameMenuConfiguration *initial_config =
            load_initial_menu_configuration(p->persistent_storage);

        Configuration *config =
            assemble_menu_selection_configuration(initial_config);

        UserInterfaceCustomization customization = {
            .accent_color = initial_config->accent_color,
            .rendering_mode = initial_config->rendering_mode,
            .show_help_text = initial_config->show_help_text,
        };

        auto maybe_interrupt =
            collect_configuration(p, config, &customization, false, true);

        if (maybe_interrupt) {
                delete config;
                delete initial_config;
                return maybe_interrupt;
        }
        extract_game_config(configuration, config);
        delete config;
        delete initial_config;
        return std::nullopt;
}

Game game_from_string(const char *name)
{
        if (strcmp(name, game_to_string(Game::Clean2048)) == 0)
                return Game::Clean2048;
        if (strcmp(name, game_to_string(Game::Minesweeper)) == 0)
                return Game::Minesweeper;
        if (strcmp(name, game_to_string(Game::GameOfLife)) == 0)
                return Game::GameOfLife;
        if (strcmp(name, game_to_string(Game::MainMenu)) == 0)
                return Game::MainMenu;
        if (strcmp(name, game_to_string(Game::Settings)) == 0)
                return Game::Settings;
        if (strcmp(name, game_to_string(Game::Snake)) == 0)
                return Game::Snake;
        if (strcmp(name, game_to_string(Game::SnakeDuel)) == 0)
                return Game::SnakeDuel;
        if (strcmp(name, game_to_string(Game::WifiApp)) == 0)
                return Game::WifiApp;
        if (strcmp(name, game_to_string(Game::RandomSeedPicker)) == 0)
                return Game::RandomSeedPicker;
        return Game::Unknown;
}

bool is_valid_game(Game game)
{
        switch (game) {
        case Game::MainMenu:
        case Game::Clean2048:
        case Game::Minesweeper:
        case Game::GameOfLife:
        case Game::Settings:
        case Game::Snake:
        case Game::SnakeDuel:
        case Game::WifiApp:
        case Game::RandomSeedPicker:
                return true;
        default:
                return false;
        }
}

const char *game_to_string(Game game)
{
        switch (game) {
        case Game::MainMenu:
                return "Main Menu";
        case Game::Clean2048:
                return "2048";
        case Game::Minesweeper:
                return "Minesweeper";
        case Game::GameOfLife:
                return "Game Of Life";
        case Game::Settings:
                return "Settings";
        case Game::Snake:
                return "Snake";
        case Game::SnakeDuel:
                return "Snake Duel";
        case Game::WifiApp:
                return "Wi-Fi";
        case Game::RandomSeedPicker:
                return "Randomness";
        default:
                return "Unknown";
        }
}
