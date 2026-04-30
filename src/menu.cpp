#include <cstring>
#include <optional>
#include <stdlib.h>

#include "menu.hpp"
#include "application_executor.hpp"

#include "common/configuration.hpp"
#include "common/logging.hpp"
#include "common/color.hpp"
#include "common/constants.hpp"

#include "games/minesweeper.hpp"
#include "games/sudoku.hpp"
#include "games/game_of_life.hpp"
#include "games/snake.hpp"
#include "games/snake_duel.hpp"
#include "games/2048.hpp"

#include "apps/app_menu.hpp"
#include "apps/power.hpp"
#include "apps/wifi.hpp"
#include "apps/brightness.hpp"
#include "apps/settings.hpp"
#include "apps/random_seed_picker.hpp"

#define TAG "game_menu"

GameMenuConfiguration DEFAULT_MENU_CONFIGURATION = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1},
    .game = Game::GameOfLife,
    .accent_color = DarkBlue,
    .rendering_mode = Minimalistic,
    .show_help_text = true,
};

const char *game_to_string(Game game);

std::optional<UserAction> select_app_and_run(Platform *p)
{
        GameMenuConfiguration config;

        auto maybe_interrupt = main_menu_interaction_loop(p, &config);

        // This customization might not be initialized properly if the user
        // requests help message. The current version of the help text rendering
        // does not depend on it but this might become problematic in the
        // future.
        UserInterfaceCustomization c = {
            config.accent_color, config.rendering_mode, config.show_help_text};

        const char *help_text =
            "Move joystick up/down to switch between menu options. Move "
            "joystick left/right or press the 'down' button to change the "
            "value of the "
            "current option. Press the right button to start the game";
        if (maybe_interrupt.has_value() &&
            maybe_interrupt.value() == UserAction::ShowHelp) {
                render_wrapped_help_text(p, &c, help_text);
                return wait_until_green_pressed(p);
        }

        // This is needed to handle the 'close window' action.
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt;
        }

        /**
         * Below we need to use this repeated pattern of calling `execute_app`
         * in each branch of the switch statement, this is because this execute
         * subroutine is a templated method and depending on what class we
         * pass into it, it resolves the config struct that will be loaded
         * from the configuration storage. Because of this, we cannot simply
         * use a switch to get an instance of ApplicationExecutor<T> as this T
         * would be different in each branch and so we then cannot pass that
         * through a single call to `execute_app`.
         */
        LOG_INFO(TAG, "User selected game: %s.", game_to_string(config.game));
        switch (config.game) {
        case Game::Clean2048:
                return execute_app(new Clean2048(), p, &c);
        case Game::Minesweeper:
                return execute_app(new Minesweeper(), p, &c);
        case Game::GameOfLife:
                return execute_app(new GameOfLife(), p, &c);
        case Game::Settings:
                return execute_app(new UtilityApplicationMenu(), p, &c);
        case Game::Snake:
                return execute_app(new SnakeGame(), p, &c);
        case Game::SnakeDuel:
                return execute_app(new SnakeDuel(), p, &c);
        case Game::WifiApp:
                return execute_app(new WifiApp(), p, &c);
        case Game::RandomSeedPicker:
                return execute_app(new RandomSeedPicker(), p, &c);
        case Game::Sudoku:
                return execute_app(new SudokuGame(), p, &c);
        case Game::Power:
                return execute_app(new PowerManagementApp(), p, &c);
        case Game::Brightness:
                return execute_app(new BrightnessApp(), p, &c);
        default:
                LOG_DEBUG(TAG, "Unsupported game selected, exiting...");
                return UserAction::Exit;
        }
}

GameMenuConfiguration *
load_initial_menu_configuration(PersistentStorage *storage);
Configuration *
assemble_game_selector_configuration(Platform *p,
                                     GameMenuConfiguration *initial_config);
void extract_app_selection(GameMenuConfiguration *menu_configuration,
                           Configuration *config);

/**
 * As a quality of life improvement, when the game console starts up, it will
 * load the default game from the memory, however when you change the game
 * and then play it, going back to the main menu will select this game by
 * default this is accomplished using this static global variable.
 */
static std::optional<Game> last_selected_game = std::nullopt;

/**
 * Responsible for getting input from the user when they first turn on the
 * console. This allows for selecting the game and navigating to the setting
 * apps if needed.
 */
std::optional<UserAction>
main_menu_interaction_loop(Platform *p, GameMenuConfiguration *configuration)
{

        GameMenuConfiguration *initial_config =
            load_initial_menu_configuration(p->persistent_storage);

        LOG_DEBUG(TAG, "Initial configuration was loaded.");

        // As explained above, if the user changes the game, going back to the
        // main menu will result in the menu having this game selected and
        // not the initial config game. This is to avoid users having to change
        // the selected game multiple times if they want to play the same thing.
        if (last_selected_game.has_value()) {
                initial_config->game = last_selected_game.value();
        }

        Configuration *config =
            assemble_game_selector_configuration(p, initial_config);

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
        extract_app_selection(configuration, config);
        last_selected_game = configuration->game;

        /*
         * Note that we are only extracting the selected game from the user
         * input and we are not setting anything else. This causes the
         * color and hints visibility settings to fall back to defaults and
         * here we need to set those again from the initial config.
         */
        configuration->accent_color = initial_config->accent_color;
        configuration->rendering_mode = initial_config->rendering_mode;
        configuration->show_help_text = initial_config->show_help_text;

        delete config;
        delete initial_config;
        return std::nullopt;
}

GameMenuConfiguration *
load_initial_menu_configuration(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offset(Game::MainMenu);

        GameMenuConfiguration configuration{};
        // When running on the emulator, if there is no persistent storage file,
        // the storage retuns back the struct object and fails silently.
        // This then tricks the magic validation below into thinking that our
        // persistent storage is good. To avoid this, we are setting magic to be
        // incorrect to detect the missing file.
        configuration.header.magic = -1;

        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, configuration);

        GameMenuConfiguration *output = new GameMenuConfiguration();

        if (!configuration.header.has_valid_magic()) {
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
assemble_game_selector_configuration(Platform *p,
                                     GameMenuConfiguration *initial_config)
{

        std::vector<const char *> available_games = {
            game_to_string(Game::Clean2048),  game_to_string(Game::Minesweeper),
            game_to_string(Game::GameOfLife), game_to_string(Game::Snake),
            game_to_string(Game::SnakeDuel),  game_to_string(Game::Sudoku),
            game_to_string(Game::Settings), game_to_string(Game::Power)
        };

        auto *game = ConfigurationOption::of_strings(
            "Game", available_games, game_to_string(initial_config->game));

        auto options = {game};

        return new Configuration("MicroBox", options);
}

void extract_app_selection(GameMenuConfiguration *menu_configuration,
                           Configuration *config)
{
        ConfigurationOption *game_option = config->options[0];

        menu_configuration->game =
            game_from_string(game_option->get_current_str_value());
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
        if (strcmp(name, game_to_string(Game::Sudoku)) == 0)
                return Game::Sudoku;
        if (strcmp(name, game_to_string(Game::Power)) == 0)
                return Game::Power;
        if (strcmp(name, game_to_string(Game::Brightness)) == 0)
                return Game::Brightness;
        if (strcmp(name, game_to_string(Game::DefaultsSetting)) == 0)
                return Game::DefaultsSetting;
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
        case Game::Sudoku:
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
        case Game::Sudoku:
                return "Sudoku";
        case Game::RandomSeedPicker:
                return "Randomness";
        case Game::Power:
                return "Power Off";
        case Game::Brightness:
                return "Brightness";
        case Game::DefaultsSetting:
                return "Defaults";
        default:
                return "Unknown";
        }
}

Configuration *
assemble_menu_defaults_configuration(Platform *p,
                                     GameMenuConfiguration *initial_config)
{

        std::vector<const char *> available_games = {
            game_to_string(Game::Minesweeper),
            game_to_string(Game::Clean2048),
            game_to_string(Game::GameOfLife),
            game_to_string(Game::Snake),
            game_to_string(Game::SnakeDuel),
            game_to_string(Game::Sudoku),
            game_to_string(Game::Settings),
            game_to_string(Game::RandomSeedPicker),
            game_to_string(Game::Brightness),
        };

        if (p->capabilities.has_wifi)
                available_games.push_back(game_to_string(Game::WifiApp));
        if (p->capabilities.can_sleep)
                available_games.push_back(game_to_string(Game::Power));

        auto *game = ConfigurationOption::of_strings(
            "Game", available_games, game_to_string(initial_config->game));

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

        return new Configuration("Menu Defaults", options);
}

void extract_defaults_config(GameMenuConfiguration *menu_configuration,
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

/**
 * Allows the user to set the default properties of the main menu. Note that
 * by default this is not visible when you first start the console. Instead, we
 * only expose this screen via the settings app.
 *
 * This design was implemented based on feedback from Khemi as he made a valid
 * point that when you are using the console, you are unlikely to change the
 * accent color / hints display / UI flavour each time you want to select the
 * game. Instead, only the game should be selectable plus a special handle
 * that will take the user to the settings menu.
 *
 */
std::optional<UserAction>
collect_game_menu_defaults_config(Platform *p,
                                  GameMenuConfiguration *configuration)
{

        GameMenuConfiguration *initial_config =
            load_initial_menu_configuration(p->persistent_storage);

        LOG_DEBUG(TAG, "Initial configuration was loaded.");

        Configuration *config =
            assemble_menu_defaults_configuration(p, initial_config);

        UserInterfaceCustomization customization = {
            .accent_color = initial_config->accent_color,
            .rendering_mode = initial_config->rendering_mode,
            .show_help_text = initial_config->show_help_text,
        };

        auto maybe_interrupt =
            collect_configuration(p, config, &customization, false, false);

        if (maybe_interrupt) {
                delete config;
                delete initial_config;
                return maybe_interrupt;
        }
        extract_defaults_config(configuration, config);

        delete config;
        delete initial_config;
        return std::nullopt;
}
