#include <memory>

#include "settings.hpp"
#include "app_menu.hpp"
#include "brightness.hpp"
#include "random_seed_picker.hpp"
#include "wifi.hpp"
#include "../menu.hpp"
#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../games/game_of_life.hpp"
#include "../games/minesweeper.hpp"
#include "../games/2048.hpp"
#include "../games/snake.hpp"
#include "../games/snake_duel.hpp"
#include "../games/sudoku.hpp"

#define TAG "settings"

Configuration *assemble_settings_menu_configuration(Platform *p);
void extract_menu_setting(Game *selected_game, Configuration *config);

std::optional<UserAction>
Settings::collect_config(Platform *p, UserInterfaceCustomization *customization,
                         SettingsConfiguration *game_config)

{
        Configuration *settings_config =
            assemble_settings_menu_configuration(p);
        auto maybe_interrupt =
            collect_configuration(p, settings_config, customization);
        if (maybe_interrupt) {
                delete settings_config;
                return maybe_interrupt;
        }

        Game selected_game;
        extract_menu_setting(&selected_game, settings_config);
        game_config->selected_game = selected_game;
        delete settings_config;
        return std::nullopt;
}
const char *Settings::get_game_name() { return "Settings Menu"; }
const char *Settings::get_help_text()
{
        return "Select the game/application and adjust the settings, press RED "
               "(right) to proceed and save the settings. Press BLUE (left) at "
               "any point to exit.";
}

UserAction Settings::app_loop(Platform *p, UserInterfaceCustomization *custom,
                              const SettingsConfiguration &config)
{
        Game selected_game = config.selected_game;
        int offset = get_settings_storage_offset(selected_game);
        LOG_DEBUG(TAG, "Computed configuration storage offset for game %s: %d",
                  game_to_string(selected_game), offset);

        PersistentStorage storage = *(p->persistent_storage);

        auto is_exit_action = [](std::optional<UserAction> action) {
                return action.value() == UserAction::Exit ||
                       action.value() == UserAction::CloseWindow;
        };

        switch (selected_game) {
        case Game::MainMenu: {
                GameMenuConfiguration config;
                auto action = collect_game_menu_defaults_config(p, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Clean2048: {
                Game2048Configuration config;
                auto game = std::make_unique<Clean2048>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Minesweeper: {
                MinesweeperConfiguration config;
                auto game = std::make_unique<Minesweeper>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::GameOfLife: {
                GameOfLifeConfiguration config;
                auto game = std::make_unique<GameOfLife>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Snake: {
                SnakeConfiguration config;
                auto game = std::make_unique<SnakeGame>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::SnakeDuel: {
                SnakeDuelConfiguration config;
                auto game = std::make_unique<SnakeDuel>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::WifiApp: {
                WifiAppConfiguration config;
                auto game = std::make_unique<WifiApp>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::RandomSeedPicker: {
                RandomSeedPickerConfiguration config;
                auto game = std::make_unique<RandomSeedPicker>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Sudoku: {
                SudokuConfiguration config;
                auto game = std::make_unique<SudokuGame>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Settings: {
                AppMenuConfiguration config;
                auto game = std::make_unique<UtilityApplicationMenu>();
                auto action = game->collect_config(p, custom, &config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        default:
                return UserAction::Exit;
        }
        LOG_DEBUG(TAG, "Re-entering the settings collecting loop.");
        return UserAction::PlayAgain;
}

std::vector<int> get_settings_storage_offsets()
{
        auto ordinal = [](Game game) { return static_cast<int>(game); };

        std::map<Game, int> config_size = {
            {Game::MainMenu, sizeof(GameMenuConfiguration)},
            {Game::Clean2048, sizeof(Game2048Configuration)},
            {Game::Minesweeper, sizeof(MinesweeperConfiguration)},
            {Game::GameOfLife, sizeof(GameOfLifeConfiguration)},
            {Game::RandomSeedPicker, sizeof(RandomSeedPickerConfiguration)},
            {Game::Snake, sizeof(SnakeConfiguration)},
            {Game::SnakeDuel, sizeof(SnakeDuelConfiguration)},
            {Game::WifiApp, sizeof(WifiAppConfiguration)},
            {Game::Sudoku, sizeof(SudokuConfiguration)},
            {Game::Brightness, sizeof(BrightnessConfiguration)},
            {Game::Settings, sizeof(AppMenuConfiguration)},
        };

        std::vector<Game> games = {
            Game::MainMenu,   Game::Clean2048,        Game::Minesweeper,
            Game::GameOfLife, Game::RandomSeedPicker, Game::Snake,
            Game::SnakeDuel,  Game::WifiApp,          Game::Sudoku,
            Game::Brightness, Game::Settings,
        };

        // We make the offsets size a two element bigger as the game enum starts
        // at 1 and we skip number 5 as that is the 'Settings' app itself. We
        // also skip number 11 as that is the sleep app that currently doesn't
        // require configuration.
        std::vector<int> offset(games.size() + 3);

        /*
         * Here we are setting the beginning offsets of the configuration struct
         * for each game so that each consecutive struct starts immediately
         * after the previous one.
         */
        offset[ordinal(Game::MainMenu)] = 0;
        for (int i = 1; i < games.size(); i++) {
                Game prev = games[i - 1];
                Game game = games[i];
                int prev_idx = ordinal(prev);
                int game_idx = ordinal(game);
                offset[game_idx] = offset[prev_idx] + config_size[prev];
        }
        return offset;
}

int get_settings_storage_offset(Game game)
{
        return get_settings_storage_offsets()[static_cast<int>(game)];
}

Configuration *assemble_settings_menu_configuration(Platform *p)
{

        std::vector<const char *> available_games = {
            game_to_string(Game::MainMenu),
            game_to_string(Game::Minesweeper),
            game_to_string(Game::Clean2048),
            game_to_string(Game::GameOfLife),
            game_to_string(Game::Snake),
            game_to_string(Game::SnakeDuel),
            game_to_string(Game::RandomSeedPicker),
            game_to_string(Game::Sudoku),
        };

        if (p->capabilities.has_wifi) {
                available_games.push_back(game_to_string(Game::WifiApp));
        }

        auto *menu = ConfigurationOption::of_strings(
            "Modify", available_games, game_to_string(Game::MainMenu));

        return new Configuration("Set Defaults", {menu});
}

void extract_menu_setting(Game *selected_menu, Configuration *config)
{
        ConfigurationOption menu_option = *config->options[0];
        *selected_menu = game_from_string(menu_option.get_current_str_value());
}
