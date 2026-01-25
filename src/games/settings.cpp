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
#include "wifi.hpp"

#define TAG "settings"

Configuration *assemble_settings_menu_configuration();
void extract_menu_setting(Game *selected_game, Configuration *config);

std::optional<UserAction>
Settings::game_loop(Platform *p, UserInterfaceCustomization *custom)
{
        // We loop until the user presses the blue button on any of the
        // configuration screens.
        while (true) {
                Configuration *settings_config =
                    assemble_settings_menu_configuration();
                auto maybe_interrupt =
                    collect_configuration(p, settings_config, custom);
                if (maybe_interrupt) {
                        delete settings_config;
                        return maybe_interrupt;
                }

                Game selected_game;
                extract_menu_setting(&selected_game, settings_config);
                delete settings_config;

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
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::Clean2048: {
                        Game2048Configuration config;
                        auto action = collect_2048_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::Minesweeper: {
                        MinesweeperConfiguration config;
                        auto action =
                            collect_minesweeper_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::GameOfLife: {
                        GameOfLifeConfiguration config;
                        auto action =
                            collect_game_of_life_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::Snake: {
                        SnakeConfiguration config;
                        auto action = collect_snake_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::SnakeDuel: {
                        SnakeDuelConfiguration config;
                        auto action =
                            collect_snake_duel_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::WifiApp: {
                        WifiAppConfiguration config;
                        auto action =
                            collect_wifi_app_config(p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                case Game::RandomSeedPicker: {
                        RandomSeedPickerConfiguration config;
                        auto action = collect_random_seed_picker_config(
                            p, &config, custom);
                        if (action &&
                            (action.value() == UserAction::Exit ||
                             action.value() == UserAction::CloseWindow)) {
                                delete settings_config;
                                return action;
                        }
                        storage.put(offset, config);
                } break;
                default:
                        return std::nullopt;
                }
                LOG_DEBUG(TAG, "Re-entering the settings collecting loop.");
        }
}

std::vector<int> get_settings_storage_offsets()
{
        std::vector<int> offsets(10);
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
                                game_to_string(Game::RandomSeedPicker)};

        auto *menu = ConfigurationOption::of_strings(
            "Modify", available_games, game_to_string(Game::MainMenu));

        return new Configuration("Set Defaults", {menu});
}

void extract_menu_setting(Game *selected_menu, Configuration *config)
{
        ConfigurationOption menu_option = *config->options[0];
        *selected_menu = game_from_string(menu_option.get_current_str_value());
}
