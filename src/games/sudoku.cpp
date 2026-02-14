#include <cassert>
#include <optional>
#include "sudoku.hpp"
#include "settings.hpp"
#include "game_menu.hpp"

#include "../lib/ArduinoJson-v7.4.2.h"
#include "../common/configuration.hpp"
#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50

#define SUDOKU_GRID_SIZE 9

#define TAG "sudoku"

SudokuConfiguration DEFAULT_SUDOKU_CONFIG = {.difficulty = 1};

class SudokuCell
{
      public:
        std::optional<int> value;
        bool is_user_defined = false;

        SudokuCell(std::optional<int> value, bool is_user_defined)
            : value(value), is_user_defined(is_user_defined)
        {
        }
};

UserAction sudoku_loop(Platform *p, UserInterfaceCustomization *customization);

std::optional<UserAction>
SudokuGame::game_loop(Platform *p, UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Use the left/right buttons to select which digit you are "
            "inserting. Use the joystick to control the cursor. Press green to "
            "(re-)place the current digit.";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (sudoku_loop(p, customization)) {
                case UserAction::PlayAgain: {
                        LOG_DEBUG(TAG, "Sudoku game loop finished. "
                                       "Pausing for input ");
                        Direction dir;
                        Action act;
                        auto maybe_event = pause_until_input(
                            p->directional_controllers, p->action_controllers,
                            &dir, &act, p->time_provider, p->display);

                        // We propagate the 'close window' action here.
                        if (maybe_event.has_value() &&
                            maybe_event.value() == UserAction::CloseWindow) {
                                return maybe_event;
                        }

                        if (act == Action::BLUE) {
                                exit_requested = true;
                        }
                        break;
                }
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_DEBUG(TAG, "User requested sudoku help screen");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                case UserAction::CloseWindow:
                        return UserAction::CloseWindow;
                }
        }
        return std::nullopt;
}

void draw_sudoku_grid_frame(Platform *p,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions);

void draw_number(Platform *p, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location,
                 SudokuCell value);

std::vector<std::vector<SudokuCell>> fetch_sudoku_grid(Platform *p)
{
        ConnectionConfig config = {
            .host = "https://sudoku-api.vercel.app/api/dosuku", .port = 443};
        std::optional<std::string> response =
            p->client->get(config, "https://sudoku-api.vercel.app/api/dosuku");

        if (!response.has_value()) {
                return {};
        }

        const char *response_value = response.value().c_str();

        JsonDocument doc;
        deserializeJson(doc, response_value);

        auto sudoku_grid = doc["newboard"]["grids"][0]["value"];

        std::vector<std::vector<SudokuCell>> output;
        for (int i = 0; i < SUDOKU_GRID_SIZE; i++) {
                std::vector<SudokuCell> row;
                for (int j = 0; j < SUDOKU_GRID_SIZE; j++) {
                        int value = sudoku_grid[i][j];
                        if (value != 0) {
                                row.emplace_back(value, false);
                        } else {
                                row.emplace_back(std::nullopt, false);
                        }
                }
                output.push_back(row);
        }

        return output;
}

UserAction sudoku_loop(Platform *p, UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering sudoku game loop");
        SudokuConfiguration config;

        auto maybe_interrupt = collect_sudoku_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        SquareCellGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), SUDOKU_GRID_SIZE,
            SUDOKU_GRID_SIZE, true);

        LOG_DEBUG(TAG, "Rendering sudoku game area.");
        draw_sudoku_grid_frame(p, customization, gd);
        auto grid = fetch_sudoku_grid(p);

        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.value.has_value()) {
                                draw_number(p, customization, gd, {x, y}, cell);
                        }
                }
        }

        return UserAction::PlayAgain;
}

void draw_number(Platform *p, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location,
                 SudokuCell cell)
{

        assert(cell.value.has_value() &&
               "Only cells with values should be rendered");

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;
        int x_offset = location.x * cell_size;
        int y_offset = location.y * cell_size;

        // To ensure that the characters are centered inside of each cell,
        // we need to calculate the 'padding' around each character. This is
        // defined by the font height and width. Note that we are subtracting
        // one to take the cell border into account and make it visually
        // centered.
        int border_adjustment_offset = 1;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;

        int x_padding = (cell_size - fw) / 2 - border_adjustment_offset;
        int y_padding = (cell_size - fh) / 2 - border_adjustment_offset;

        char buffer[2];
        sprintf(buffer, "%d", cell.value.value());

        int x = x_margin + x_offset + x_padding;
        int y = y_margin + y_offset + y_padding;

        Color render_color =
            cell.is_user_defined ? White : customization->accent_color;
        p->display->draw_string({x, y}, buffer, FontSize::Size16, Black,
                                render_color);
}

void draw_sudoku_grid_frame(Platform *p,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions)

{
        p->display->initialize();
        p->display->clear(Black);

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;

        // We only render borders between cells for a minimalistic look
        for (int i = 1; i < SUDOKU_GRID_SIZE; i++) {
                int offset = i * cell_size;
                auto draw_vertical_line = [&](int offset) {
                        Point start = {.x = x_margin + offset, .y = y_margin};
                        Point end = {.x = x_margin + offset,
                                     .y = y_margin + actual_width};
                        p->display->draw_line(start, end,
                                              customization->accent_color);
                };
                auto draw_horizontal_line = [&](int offset) {
                        Point start = {.x = x_margin, .y = y_margin + offset};
                        Point end = {.x = x_margin + actual_width,
                                     .y = y_margin + offset};
                        p->display->draw_line(start, end,
                                              customization->accent_color);
                };

                draw_vertical_line(offset);
                draw_horizontal_line(offset);

                // We make the lines between big squares thicker
                if (i % 3 == 0) {
                        draw_vertical_line(offset - 1);
                        draw_horizontal_line(offset - 1);
                        draw_vertical_line(offset + 1);
                        draw_horizontal_line(offset + 1);
                }
        }
}

/**
 * Forward declarations of functions related to configuration manipulation.
 */
SudokuConfiguration *load_initial_sudoku_config(PersistentStorage *storage);
Configuration *assemble_sudoku_configuration(PersistentStorage *storage);
void extract_game_config(SudokuConfiguration *game_config,
                         Configuration *config);

std::optional<UserAction>
collect_sudoku_config(Platform *p, SudokuConfiguration *game_config,
                      UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_sudoku_configuration(p->persistent_storage);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                delete config;
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        delete config;
        return std::nullopt;
}

Configuration *assemble_sudoku_configuration(PersistentStorage *storage)
{
        SudokuConfiguration *initial_config =
            load_initial_sudoku_config(storage);

        ConfigurationOption *difficulty = ConfigurationOption::of_integers(
            "Difficulty", {1, 2, 3}, initial_config->difficulty);

        std::vector<ConfigurationOption *> options = {difficulty};

        delete initial_config;
        return new Configuration("Sudoku", options);
}

SudokuConfiguration *load_initial_sudoku_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::Sudoku);
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        // We initialize empty config to detect corrupted memory and fallback
        // to defaults if needed.
        SudokuConfiguration config = {.difficulty = 0};

        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage->get(storage_offset, config);

        SudokuConfiguration *output = new SudokuConfiguration();

        if (config.difficulty == 0) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "sudoku configuration, using default values.");
                memcpy(output, &DEFAULT_SUDOKU_CONFIG,
                       sizeof(SudokuConfiguration));
                storage->put(storage_offset, DEFAULT_SUDOKU_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(SudokuConfiguration));
        }

        LOG_DEBUG(TAG, "Loaded sudoku configuration: difficulty=%d",
                  output->difficulty);

        return output;
}

void extract_game_config(SudokuConfiguration *game_config,
                         Configuration *config)
{
        ConfigurationOption difficulty = *config->options[0];

        game_config->difficulty = difficulty.get_curr_int_value();
}
