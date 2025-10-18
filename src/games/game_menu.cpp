#include <optional>
#include <stdlib.h>
#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/platform/interface/color.hpp"
#include "2048.hpp"
#include "game_executor.hpp"
#include "minesweeper.hpp"
#include "settings.hpp"
#include "game_of_life.hpp"
#include "snake.hpp"

#define TAG "game_menu"

GameMenuConfiguration DEFAULT_MENU_CONFIGURATION = {
    .game = GameOfLife,
    .accent_color = DarkBlue,
    .rendering_mode = Minimalistic,
    .show_help_text = true,
};

const char *game_to_string(Game game);

GameMenuConfiguration *
load_initial_menu_configuration(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offsets()[MainMenu];

        GameMenuConfiguration configuration = {.game = Unknown,
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
             game_to_string(Game::Settings)},
            game_to_string(initial_config->game));

        auto available_colors = {
            Color::Red,      Color::Green,      Color::Blue,  Color::DarkBlue,
            Color::Magenta,  Color::Cyan,       Color::Gblue, Color::Brown,
            Color::Yellow,   Color::BRRed,      Color::Gray,  Color::LightBlue,
            Color::GrayBlue, Color::LightGreen, Color::LGray, Color::LGrayBlue,
            Color::LBBlue};

        auto *accent_color = ConfigurationOption::of_colors(
            "Color", available_colors, initial_config->accent_color);

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

        return new Configuration("MicroBox", options, "Select game");
}

void extract_game_config(GameMenuConfiguration *menu_configuration,
                         Configuration *config)
{
        ConfigurationOption game_option = *config->options[0];

        int curr_option_idx = game_option.currently_selected;
        menu_configuration->game = game_from_string(static_cast<const char **>(
            game_option.available_values)[curr_option_idx]);

        ConfigurationOption accent_color = *config->options[1];

        int curr_accent_color_idx = accent_color.currently_selected;
        Color color = static_cast<Color *>(
            accent_color.available_values)[curr_accent_color_idx];
        menu_configuration->accent_color = color;

        ConfigurationOption rendering_mode = *config->options[2];

        int curr_mode_idx = rendering_mode.currently_selected;
        const char *mode_str = static_cast<const char **>(
            rendering_mode.available_values)[curr_mode_idx];
        menu_configuration->rendering_mode = rendering_mode_from_str(mode_str);

        ConfigurationOption show_help_text = *config->options[3];

        int show_help_text_idx = show_help_text.currently_selected;
        const char *yes_or_no = static_cast<const char **>(
            show_help_text.available_values)[show_help_text_idx];
        menu_configuration->show_help_text =
            extract_yes_or_no_option(yes_or_no);
}

void select_game(Platform *p)
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
                wait_until_green_pressed(p);
                return;
        }

        LOG_INFO(TAG, "User selected game: %s.", game_to_string(config.game));

        GameExecutor *executor = [&]() -> GameExecutor * {
                switch (config.game) {
                case Unknown:
                case Clean2048:
                        return new class Clean2048();
                case Minesweeper:
                        return new class Minesweeper();
                case GameOfLife:
                        return new class GameOfLife();
                case Settings:
                        return new class Settings();
                case Snake:
                        return new class Snake();
                default:
                        return NULL;
                }
        }();

        if (!executor) {
                LOG_DEBUG(TAG, "Selected game: %d. Game not implemented yet.",
                          config.game);
                return;
        }

        executor->game_loop(p, &customization);
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
                return maybe_interrupt;
        }

        extract_game_config(configuration, config);

        free_configuration(config);
        free(initial_config);
        return std::nullopt;
}

Game game_from_string(const char *name)
{
        if (strcmp(name, game_to_string(Clean2048)) == 0)
                return Game::Clean2048;
        if (strcmp(name, game_to_string(Minesweeper)) == 0)
                return Game::Minesweeper;
        if (strcmp(name, game_to_string(GameOfLife)) == 0)
                return Game::GameOfLife;
        if (strcmp(name, game_to_string(MainMenu)) == 0)
                return Game::MainMenu;
        if (strcmp(name, game_to_string(Settings)) == 0)
                return Game::Settings;
        if (strcmp(name, game_to_string(Snake)) == 0)
                return Game::Snake;
        return Game::Unknown;
}

bool is_valid_game(Game game)
{
        switch (game) {
        case MainMenu:
        case Clean2048:
        case Minesweeper:
        case GameOfLife:
        case Settings:
        case Snake:
                return true;
        default:
                return false;
        }
}

const char *game_to_string(Game game)
{
        switch (game) {
        case MainMenu:
                return "Main Menu";
        case Clean2048:
                return "2048";
        case Minesweeper:
                return "Minesweeper";
        case GameOfLife:
                return "Game Of Life";
        case Settings:
                return "Settings";
        case Snake:
                return "Snake";
        default:
                return "Unknown";
        }
}
