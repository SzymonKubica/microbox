#include "2048.hpp"
#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "settings.hpp"
#include "game_of_life.hpp"
#include "minesweeper.hpp"
#include "random_seed_picker.hpp"
#include "snake.hpp"
#include "snake_duel.hpp"
#include "sudoku.hpp"
#include "wifi.hpp"

#define TAG "settings"

Configuration *assemble_settings_menu_configuration();
void extract_menu_setting(Game *selected_game, Configuration *config);

std::optional<UserAction>
Settings::collect_config(Platform *p, UserInterfaceCustomization *customization,
                         SettingsConfiguration *game_config)

{
        Configuration *settings_config = assemble_settings_menu_configuration();
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
               "to "
               "proceed and save the settings. Press BLUE at any point to "
               "exit.";
}

UserAction Settings::app_loop(Platform *p, UserInterfaceCustomization *custom,
                              const SettingsConfiguration &config)
{
        Game selected_game = config.selected_game;
        // We loop until the user presses the blue button on any of the
        // configuration screens.
        while (true) {
                int offset = get_settings_storage_offset(selected_game);
                LOG_DEBUG(
                    TAG,
                    "Computed configuration storage offset for game %s: %d",
                    game_to_string(selected_game), offset);

                PersistentStorage storage = *(p->persistent_storage);

                switch (selected_game) {
                case Game::MainMenu: {
                        GameMenuConfiguration config;
                        auto action = collect_game_menu_config(p, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::Clean2048: {
                        Game2048Configuration config;
                        auto action = (new Clean2048())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::Minesweeper: {
                        MinesweeperConfiguration config;
                        auto action = (new Minesweeper())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::GameOfLife: {
                        GameOfLifeConfiguration config;
                        auto action = (new GameOfLife())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::Snake: {
                        SnakeConfiguration config;
                        auto action = (new SnakeGame())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::SnakeDuel: {
                        SnakeDuelConfiguration config;
                        auto action = (new SnakeDuel())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::WifiApp: {
                        WifiAppConfiguration config;
                        auto action =
                            (new WifiApp())->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::RandomSeedPicker: {
                        RandomSeedPickerConfiguration config;
                        // TODO: clean up to remove those allocations
                        auto action = (new RandomSeedPicker())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                case Game::Sudoku: {
                        SudokuConfiguration config;
                        // TODO: clean up to remove those allocations
                        auto action = (new SudokuGame())
                                          ->collect_config(p, custom, &config);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                return action.value();
                        }
                        storage.put(offset, config);
                } break;
                default:
                        return UserAction::Exit;
                }
                LOG_DEBUG(TAG, "Re-entering the settings collecting loop.");
        }
}

std::vector<int> get_settings_storage_offsets()
{
        std::vector<int> offsets(11);
        offsets[static_cast<int>(Game::MainMenu)] = 0;
        offsets[static_cast<int>(Game::Clean2048)] =
            offsets[static_cast<int>(Game::MainMenu)] +
            sizeof(GameMenuConfiguration);
        offsets[static_cast<int>(Game::Minesweeper)] =
            offsets[static_cast<int>(Game::Clean2048)] +
            sizeof(Game2048Configuration);
        offsets[static_cast<int>(Game::GameOfLife)] =
            offsets[static_cast<int>(Game::Minesweeper)] +
            sizeof(MinesweeperConfiguration);
        offsets[static_cast<int>(Game::RandomSeedPicker)] =
            offsets[static_cast<int>(Game::GameOfLife)] +
            sizeof(GameOfLifeConfiguration);
        offsets[static_cast<int>(Game::Snake)] =
            offsets[static_cast<int>(Game::RandomSeedPicker)] +
            sizeof(RandomSeedPickerConfiguration);
        offsets[static_cast<int>(Game::SnakeDuel)] =
            offsets[static_cast<int>(Game::Snake)] + sizeof(SnakeConfiguration);
        offsets[static_cast<int>(Game::WifiApp)] =
            offsets[static_cast<int>(Game::SnakeDuel)] +
            sizeof(SnakeDuelConfiguration);
        offsets[static_cast<int>(Game::Sudoku)] =
            offsets[static_cast<int>(Game::WifiApp)] +
            sizeof(WifiAppConfiguration);
        return offsets;
}

int get_settings_storage_offset(Game game)
{
        return get_settings_storage_offsets()[static_cast<int>(game)];
}

Configuration *assemble_settings_menu_configuration()
{

        auto available_games = {game_to_string(Game::MainMenu),
                                game_to_string(Game::Minesweeper),
                                game_to_string(Game::Clean2048),
                                game_to_string(Game::GameOfLife),
                                game_to_string(Game::Snake),
                                game_to_string(Game::SnakeDuel),
        // Disable the wifi functionality on the Uno R4 Minima
#if defined(ARDUINO_UNOR4_WIFI) || defined(EMULATOR)
                                game_to_string(Game::WifiApp),
#endif
                                game_to_string(Game::RandomSeedPicker),
                                game_to_string(Game::Sudoku)};

        auto *menu = ConfigurationOption::of_strings(
            "Modify", available_games, game_to_string(Game::MainMenu));

        return new Configuration("Set Defaults", {menu});
}

void extract_menu_setting(Game *selected_menu, Configuration *config)
{
        ConfigurationOption menu_option = *config->options[0];
        *selected_menu = game_from_string(menu_option.get_current_str_value());
}
