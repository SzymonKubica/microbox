#include <memory>

#include "settings.hpp"
#include "app_menu.hpp"
#include "brightness.hpp"
#include "display_scale.hpp"
#include "random_seed_picker.hpp"
#include "weather.hpp"
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

Configuration *assemble_settings_menu_configuration(const Platform &p);
void extract_menu_setting(Game &selection, const Configuration &config);

std::optional<UserAction>
Settings::collect_config(const Platform &p,
                         const UserInterfaceCustomization &customization,
                         SettingsConfiguration &game_config) const

{
        auto settings_config = std::unique_ptr<Configuration>(
            assemble_settings_menu_configuration(p));
        auto interrupt =
            collect_configuration(p, *settings_config, customization);
        if (interrupt)
                return interrupt;

        Game selected_game;
        extract_menu_setting(selected_game, *settings_config);
        game_config.selected_game = selected_game;
        return std::nullopt;
}
const char *Settings::get_game_name() const { return "Settings"; }
const char *Settings::get_help_text() const
{
        return "Select the game/application and adjust the settings, press "
               "'right' to proceed and save the settings. Press 'left' at "
               "any point to exit.";
}

UserAction Settings::app_loop(const Platform &p,
                              const UserInterfaceCustomization &custom,
                              const SettingsConfiguration &config) const
{
        Game selected_game = config.selected_game;
        int offset = get_settings_storage_offset(selected_game);
        LOG_DEBUG(TAG, "Computed configuration storage offset for game %s: %d",
                  GameStr::to_cstr(selected_game), offset);

        const PersistentStorage &storage = *(p.persistent_storage);

        auto is_exit_action = [](std::optional<UserAction> action) {
                return action.value() == UserAction::Exit ||
                       action.value() == UserAction::CloseWindow;
        };

        switch (selected_game) {
        case Game::MainMenu: {
                GameMenuConfiguration config;
                auto action = collect_game_menu_defaults_config(p, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Clean2048: {
                Game2048Configuration config;
                auto game = std::make_unique<Clean2048>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Minesweeper: {
                MinesweeperConfiguration config;
                auto game = std::make_unique<Minesweeper>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::GameOfLife: {
                GameOfLifeConfiguration config;
                auto game = std::make_unique<GameOfLife>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Snake: {
                SnakeConfiguration config;
                auto game = std::make_unique<SnakeGame>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::SnakeDuel: {
                SnakeDuelConfiguration config;
                auto game = std::make_unique<SnakeDuel>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::WifiApp: {
                WifiAppConfiguration config;
                auto game = std::make_unique<WifiApp>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::RandomSeedPicker: {
                RandomSeedPickerConfiguration config;
                auto game = std::make_unique<RandomSeedPicker>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Sudoku: {
                SudokuConfiguration config;
                auto game = std::make_unique<SudokuGame>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::Settings: {
                AppMenuConfiguration config;
                auto game = std::make_unique<UtilityApplicationMenu>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::DisplaySizeSetting: {
                DisplayScaleConfiguration config;
                auto game = std::make_unique<DisplayScaleApp>();
                auto action = game->collect_config(p, custom, config);
                if (action && is_exit_action(action))
                        return action.value();
                storage.put(offset, config);
        } break;
        case Game::WeatherApp: {
                WeatherAppConfiguration config;
                auto game = std::make_unique<WeatherApp>();
                auto action = game->collect_config(p, custom, config);
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
            {Game::DisplaySizeSetting, sizeof(DisplayScaleConfiguration)},
            {Game::WeatherApp, sizeof(WeatherAppConfiguration)},
        };

        std::vector<Game> games = {
            Game::MainMenu,   Game::Clean2048,        Game::Minesweeper,
            Game::GameOfLife, Game::RandomSeedPicker, Game::Snake,
            Game::SnakeDuel,  Game::WifiApp,          Game::Sudoku,
            Game::Brightness, Game::Settings,         Game::DisplaySizeSetting,
            Game::WeatherApp};

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

Configuration *assemble_settings_menu_configuration(const Platform &p)
{

        std::vector<const char *> available_games = {
            GameStr::to_cstr(Game::MainMenu),
            GameStr::to_cstr(Game::Minesweeper),
            GameStr::to_cstr(Game::Clean2048),
            GameStr::to_cstr(Game::GameOfLife),
            GameStr::to_cstr(Game::Snake),
            GameStr::to_cstr(Game::SnakeDuel),
            GameStr::to_cstr(Game::RandomSeedPicker),
            GameStr::to_cstr(Game::Sudoku),
        };

        if (p.capabilities.has_wifi) {
                available_games.push_back(GameStr::to_cstr(Game::WifiApp));
                available_games.push_back(GameStr::to_cstr(Game::WeatherApp));
        }

        if (p.capabilities.has_resizable_display) {
                available_games.push_back(
                    GameStr::to_cstr(Game::DisplaySizeSetting));
        }

        auto *menu = ConfigurationOption::of_strings(
            "Modify", available_games, GameStr::to_cstr(Game::MainMenu));

        return new Configuration("Set Defaults", {menu});
}

void extract_menu_setting(Game &selection, const Configuration &config)
{
        ConfigurationOption menu_option = *config.options[0];
        selection =
            GameStr::from_cstr(menu_option.get_current_str_value()).value();
}

void Settings::render_thumbnail(const Platform &platform,
                                const UserInterfaceCustomization &customization)
{
        const auto &display = *platform.display;
        clear_half_display_and_render_subtitle(platform, customization,
                                               "Settings");

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();

        // [BEGIN lopaka generated]

        auto draw_polygon_10_copy_4 = [&]() {
                tft.drawLine(132, 156, 146, 142, 0xBDF7);
                tft.drawLine(146, 142, 154, 150, 0xBDF7);
                tft.drawLine(154, 150, 140, 164, 0xBDF7);
                tft.drawLine(140, 164, 132, 156, 0xBDF7);
        };

        auto draw_polygon_10_copy_3 = [&]() {
                tft.drawLine(166, 124, 180, 110, 0xBDF7);
                tft.drawLine(180, 110, 188, 118, 0xBDF7);
                tft.drawLine(188, 118, 174, 132, 0xBDF7);
                tft.drawLine(174, 132, 166, 124, 0xBDF7);
        };

        auto draw_polygon_10 = [&]() {
                tft.drawLine(132, 118, 140, 110, 0xBDF7);
                tft.drawLine(140, 110, 145, 115, 0xBDF7);
                tft.drawLine(145, 115, 137, 123, 0xBDF7);
                tft.drawLine(137, 123, 132, 118, 0xBDF7);
        };

        auto draw_polygon_10_copy_1 = [&]() {
                tft.drawLine(175, 160, 183, 152, 0xBDF7);
                tft.drawLine(183, 152, 188, 157, 0xBDF7);
                tft.drawLine(188, 157, 180, 165, 0xBDF7);
                tft.drawLine(180, 165, 175, 160, 0xBDF7);
        };

        auto draw_polygon_10_copy_2 = [&]() {
                tft.drawLine(175, 160, 183, 152, 0xBDF7);
                tft.drawLine(183, 152, 188, 157, 0xBDF7);
                tft.drawLine(188, 157, 180, 165, 0xBDF7);
                tft.drawLine(180, 165, 175, 160, 0xBDF7);
        };
        // rect 4 copy 2
        tft.drawRect(148, 125, 22, 22, 0x4208);
        // ellipse 1
        tft.fillCircle(160, 137, 27, 0xBDF7);
        // rect 4 copy 6
        tft.drawRect(185, 131, 9, 13, 0xBDF7);
        // rect 4 copy 7
        tft.fillRect(185, 131, 9, 13, 0xBDF7);
        // polygon 10 copy 4
        draw_polygon_10_copy_4();
        // polygon 10 copy 3
        draw_polygon_10_copy_3();
        // ellipse 2
        tft.fillCircle(160, 137, 15, 0x0);
        // rect 4 copy 4
        tft.drawRect(154, 104, 13, 9, 0xBDF7);
        // rect 4 copy 5
        tft.fillRect(127, 131, 9, 13, 0xBDF7);
        // rect 4 copy 8
        tft.fillRect(154, 104, 13, 9, 0xBDF7);
        // rect 4 copy 9
        tft.fillRect(154, 162, 13, 9, 0xBDF7);
        // polygon 10
        draw_polygon_10();
        // polygon 10 copy 1
        draw_polygon_10_copy_1();
        // polygon 10 copy 2
        draw_polygon_10_copy_2();
        // triangle 15
        tft.fillTriangle(140, 110, 133, 118, 147, 118, 0xBDF7);
        // triangle 15 copy 1
        tft.fillTriangle(180, 110, 174, 117, 186, 117, 0xBDF7);
        // triangle 15 copy 2
        tft.fillTriangle(181, 150, 175, 157, 187, 157, 0xBDF7);
        // triangle 15 copy 3
        tft.fillTriangle(139, 149, 133, 156, 145, 156, 0xBDF7);
        // triangle 19
        tft.fillTriangle(142, 126, 150, 118, 133, 118, 0xBDF7);
        // triangle 19 copy 1
        tft.fillTriangle(180, 125, 187, 117, 173, 117, 0xBDF7);
        // triangle 19 copy 2
        tft.fillTriangle(180, 164, 187, 156, 173, 156, 0xBDF7);
        // triangle 19 copy 3
        tft.fillTriangle(140, 163, 147, 155, 133, 155, 0xBDF7);
        // [END lopaka generated]
}
