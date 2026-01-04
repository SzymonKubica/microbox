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

void Settings::game_loop(Platform *p, UserInterfaceCustomization *custom)
{
        // We loop until the user presses the blue button on any of the
        // configuration screens.
        while (true) {
                Configuration *config = assemble_settings_menu_configuration();
                if (collect_configuration(p, config, custom))
                        return;

                Game selected_game;
                extract_menu_setting(&selected_game, config);

                std::vector<int> offsets = get_settings_storage_offsets();
                int offset = offsets[selected_game];
                LOG_DEBUG(
                    TAG,
                    "Computed configuration storage offset for game %s: %d",
                    game_to_string(selected_game), offset);

                PersistentStorage storage = *(p->persistent_storage);

                switch (selected_game) {
                case MainMenu: {
                        GameMenuConfiguration config;
                        auto action = collect_game_menu_config(p, &config);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case Clean2048: {
                        Game2048Configuration config;
                        auto action = collect_2048_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case Minesweeper: {
                        MinesweeperConfiguration config;
                        auto action =
                            collect_minesweeper_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case GameOfLife: {
                        GameOfLifeConfiguration config;
                        auto action =
                            collect_game_of_life_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case Snake: {
                        SnakeConfiguration config;
                        auto action = collect_snake_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case SnakeDuel: {
                        SnakeDuelConfiguration config;
                        auto action =
                            collect_snake_duel_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case WifiApp: {
                        WifiAppConfiguration config;
                        auto action =
                            collect_wifi_app_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                case RandomSeedPicker: {
                        RandomSeedPickerConfiguration config;
                        auto action =
                            collect_random_seed_picker_config(p, &config, custom);
                        if (action && action.value() == UserAction::Exit) {
                                return;
                        }
                        storage.put(offset, config);
                } break;
                default:
                        return;
                }
                LOG_DEBUG(TAG, "Re-entering the settings collecting loop.");
        }
}

std::vector<int> get_settings_storage_offsets()
{
        std::vector<int> offsets(10);
        offsets[MainMenu] = 0;
        offsets[Clean2048] = offsets[MainMenu] + sizeof(GameMenuConfiguration);
        offsets[Minesweeper] =
            offsets[Clean2048] + sizeof(Game2048Configuration);
        offsets[GameOfLife] =
            offsets[Minesweeper] + sizeof(MinesweeperConfiguration);
        offsets[RandomSeedPicker] =
            offsets[GameOfLife] + sizeof(GameOfLifeConfiguration);
        offsets[Snake] =
            offsets[RandomSeedPicker] + sizeof(RandomSeedPickerConfiguration);
        offsets[SnakeDuel] = offsets[Snake] + sizeof(SnakeConfiguration);
        offsets[WifiApp] = offsets[SnakeDuel] + sizeof(SnakeDuelConfiguration);
        return offsets;
}
Configuration *assemble_settings_menu_configuration()
{

        auto available_games = {
            game_to_string(Game::MainMenu),  game_to_string(Game::Minesweeper),
            game_to_string(Game::Clean2048), game_to_string(Game::GameOfLife),
            game_to_string(Game::Snake),     game_to_string(Game::SnakeDuel),
            game_to_string(Game::WifiApp), game_to_string(Game::RandomSeedPicker)};

        auto *menu = ConfigurationOption::of_strings(
            "Modify", available_games, game_to_string(Game::MainMenu));

        return new Configuration("Set Defaults", {menu});
}

void extract_menu_setting(Game *selected_menu, Configuration *config)
{
        ConfigurationOption menu_option = *config->options[0];
        *selected_menu = game_from_string(menu_option.get_current_str_value());
}
