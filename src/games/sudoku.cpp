#include <cassert>
#include <memory>
#include <optional>
#include <algorithm>
#include <random>
#include "sudoku.hpp"
#include "settings.hpp"
#include "game_menu.hpp"

#include "../lib/ArduinoJson-v7.4.2.h"
#include "../common/configuration.hpp"
#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"
#include "../common/maths_utils.hpp"

#define GAME_LOOP_DELAY 50
#define CONTROL_POLLING_DELAY 10

#define SUDOKU_GRID_SIZE 9

#define TAG "sudoku"

SudokuConfiguration DEFAULT_SUDOKU_CONFIG = {.difficulty = 1};

class SudokuCell
{
      public:
        std::optional<int> value;
        bool is_user_defined = true;

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
                 const SudokuCell &value);

void erase_number(Platform *p, UserInterfaceCustomization *customization,
                  SquareCellGridDimensions *dimensions, Point location);

std::optional<Point>
find_empty_cell(const std::vector<std::vector<SudokuCell>> &grid)
{
        int x = 0;
        int y = 0;
        for (auto &row : grid) {
                x = 0;
                for (auto &cell : row) {
                        if (cell.is_user_defined && !cell.value.has_value()) {
                                Point location = {x, y};
                                return location;
                        }
                        x++;
                }
                y++;
        }
        return std::nullopt;
}

/**
 * Applies sudoku rules to determine the set of candidate numbers that can be
 * placed on the `location` in the grid.
 */
std::vector<int> find_valid_numbers(std::vector<std::vector<SudokuCell>> &grid,
                                    Point location)
{
        std::vector<int> candidates = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto remove_candidate = [&](int candidate_digit) {
                auto position = std::find(candidates.begin(), candidates.end(),
                                          candidate_digit);
                if (position != candidates.end()) {
                        candidates.erase(position);
                }
        };

        // Check row
        for (int x = 0; x < 9; x++) {
                if (x == location.x) {
                        continue;
                }
                auto &current = grid[location.y][x];
                if (current.value.has_value()) {
                        remove_candidate(current.value.value());
                }
        }

        // Check column
        for (int y = 0; y < 9; y++) {
                if (y == location.y) {
                        continue;
                }
                auto &current = grid[y][location.x];
                if (current.value.has_value()) {
                        remove_candidate(current.value.value());
                }
        }

        // Check big square
        Point square_top_left_corner = {3 * (location.x / 3),
                                        3 * (location.y / 3)};

        for (int y = square_top_left_corner.y; y < square_top_left_corner.y + 3;
             y++) {
                for (int x = square_top_left_corner.x;
                     x < square_top_left_corner.x + 3; x++) {
                        if (x == location.x && y == location.y) {
                                continue;
                        }
                        auto &current = grid[y][x];
                        if (current.value.has_value()) {
                                remove_candidate(current.value.value());
                        }
                }
        }

        return candidates;
}

/**
 * Implements uniform random number generator (URBG) 'interface'
 */
class RandomGenerator
{
      public:
        using result_type = uint32_t;
        result_type operator()() { return rand(); }
        static constexpr result_type min() { return 0; }
        static constexpr result_type max()
        {
                return static_cast<uint32_t>(RAND_MAX);
        }
};

bool populate_solved_grid(std::vector<std::vector<SudokuCell>> &grid)
{
        std::optional<Point> maybe_empty = find_empty_cell(grid);
        if (!maybe_empty.has_value()) {
                return true;
        }

        Point empty = maybe_empty.value();
        LOG_DEBUG(TAG, "Found empty cell: {x: %d, y: %d}", empty.x, empty.y);
        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);

        std::shuffle(valid_numbers.begin(), valid_numbers.end(),
                     RandomGenerator{});

        for (int candidate : valid_numbers) {
                grid[empty.y][empty.x].value = candidate;
                if (populate_solved_grid(grid)) {
                        return true;
                }
                grid[empty.y][empty.x].value = std::nullopt;
        }

        return false;
}

std::vector<std::vector<SudokuCell>> generate_solved_grid()
{
        std::vector<std::vector<SudokuCell>> grid(
            SUDOKU_GRID_SIZE,
            std::vector(SUDOKU_GRID_SIZE, SudokuCell(std::nullopt, true)));

        populate_solved_grid(grid);
        return grid;
}
bool test_for_unique_solution(std::vector<std::vector<SudokuCell>> &grid,
                              std::unique_ptr<int> &solution_count);

std::vector<std::vector<SudokuCell>> generate_solvable_grid()
{
        auto solvable = generate_solved_grid();

        // TODO: tweak this depending on the difficulty level.
        int to_remove = 40;

        std::vector<Point> locations_to_remove;

        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        locations_to_remove.push_back({x, y});
                }
        }

        // We shuffle the candidate numbers positions to ensure that the
        // generated empty number pattern is random.
        std::shuffle(locations_to_remove.begin(), locations_to_remove.end(),
                     RandomGenerator{});

        int candidate_idx = 0;
        int removed = 0;

        while (removed < to_remove) {
                Point loc = locations_to_remove[candidate_idx];
                int x = loc.x;
                int y = loc.y;

                int previous_value = solvable[y][x].value.value();
                solvable[y][x].value = std::nullopt;
                solvable[y][x].is_user_defined = true;
                std::vector<std::vector<SudokuCell>> clone = solvable;

                std::unique_ptr<int> solution_count = std::make_unique<int>(0);

                test_for_unique_solution(clone, solution_count);

                if (*solution_count.get() > 1) {
                        solvable[y][x].value = previous_value;
                        solvable[y][x].is_user_defined = false;
                } else {
                        removed++;
                }
                candidate_idx++;
        }

        // When the grid is generated, we need to make all cells
        // non-user-defined.
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        if (solvable[y][x].value.has_value()) {
                                solvable[y][x].is_user_defined = false;
                        }
                }
        }

        return solvable;
}

bool test_for_unique_solution(std::vector<std::vector<SudokuCell>> &grid,
                              std::unique_ptr<int> &solution_count)
{

        // Prune the search space to return true immediately once more than 1
        // solution is found.
        if (*solution_count.get() > 1) {
                return true;
        }

        std::optional<Point> maybe_empty = find_empty_cell(grid);
        if (!maybe_empty.has_value()) {
                (*solution_count.get())++;
                return true;
        }

        Point empty = maybe_empty.value();
        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);

        for (int candidate : valid_numbers) {
                grid[empty.y][empty.x].value = candidate;
                if (test_for_unique_solution(grid, solution_count)) {
                        return true;
                }
                grid[empty.y][empty.x].value = std::nullopt;
        }

        return false;
}

bool solve(std::vector<std::vector<SudokuCell>> &grid)
{
        std::optional<Point> maybe_empty = find_empty_cell(grid);
        if (!maybe_empty.has_value()) {
                return true;
        }

        Point empty = maybe_empty.value();
        LOG_DEBUG(TAG, "Found empty cell: {x: %d, y: %d}", empty.x, empty.y);
        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);

        LOG_DEBUG(TAG, "Found valid numbers: ", "");
        for (int v : valid_numbers) {
                LOG_DEBUG(TAG, "%d ", v);
        }

        for (int candidate : valid_numbers) {
                grid[empty.y][empty.x].value = candidate;
                if (solve(grid)) {
                        return true;
                }
                grid[empty.y][empty.x].value = std::nullopt;
        }

        return false;
}

std::vector<std::vector<SudokuCell>> fetch_sudoku_grid(Platform *p)
{
        ConnectionConfig config = {
            .host = "https://sudoku-api.vercel.app/api/dosuku", .port = 443};
        std::optional<std::string> response =
            p->client->get(config, "https://sudoku-api.vercel.app/api/dosuku");

        if (!response.has_value()) {
                std::vector<std::vector<SudokuCell>> output(
                    9, std::vector(9, SudokuCell(std::nullopt, true)));
                return output;
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
                                std::optional<int> value_opt = value;
                                row.push_back({value_opt, false});
                        } else {
                                row.push_back({std::nullopt, true});
                        }
                }
                output.push_back(row);
        }

        return output;
}

void draw_grid_numbers(Platform *p, UserInterfaceCustomization *customization,
                       SquareCellGridDimensions *dimensions,
                       const std::vector<std::vector<SudokuCell>> &grid)
{
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.value.has_value()) {
                                draw_number(p, customization, dimensions,
                                            {x, y}, cell);
                        }
                }
        }
}

/**
 * Here we repurpose the number drawing functionatlity to render the list of
 * available numbers to the left of the grid.
 */
void draw_available_numbers(Platform *p,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions,
                            const std::vector<std::vector<SudokuCell>> &grid)
{
        for (int i = 0; i < SUDOKU_GRID_SIZE; i++) {
                SudokuCell cell(i + 1, true);
                draw_number(p, customization, dimensions, {-1, i}, cell);
        }
}

void draw_circle_selector(Platform *p, SquareCellGridDimensions *dimensions,
                          int selected_number, Color color)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;
        int x_offset = -1.5 * cell_size;
        int y_offset = (selected_number - 1) * cell_size;
        int circle_radius = 3;

        int padding = (cell_size - circle_radius) / 2;
        // Need this to make the selector visually balanced and
        // centered.
        int y_adjustment = 3;

        int x = x_margin + x_offset + padding;
        int y = y_margin + y_offset + padding + y_adjustment;

        p->display->draw_circle({x, y}, circle_radius, color, 1, true);
}

void erase_circle_selector(Platform *p, SquareCellGridDimensions *dimensions,
                           int selected_number)
{
        draw_circle_selector(p, dimensions, selected_number, Black);
}

void draw_caret(Platform *p, SquareCellGridDimensions *dimensions,
                const Point &caret, Color caret_color)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;
        // The caret needs to be drawn inside of the cell and avoid
        // touching its borders. Because of this we need to shift the
        // caret inside the grid and make its sides appropriately
        // shorter.
        int offset = 3;

        int start_x = x_margin + cell_size * caret.x + offset;
        int start_y = y_margin + cell_size * caret.y + offset;

        int caret_width = cell_size - 2 * offset;

        p->display->draw_rectangle({start_x, start_y}, caret_width, caret_width,
                                   caret_color, 1, false);
}

void draw_caret(Platform *p, UserInterfaceCustomization *customization,
                SquareCellGridDimensions *dimensions, const Point &caret)
{
        draw_caret(p, dimensions, caret, White);
}

void erase_caret(Platform *p, SquareCellGridDimensions *dimensions,
                 const Point &caret)
{
        draw_caret(p, dimensions, caret, Black);
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
        auto grid = generate_solvable_grid();

        draw_grid_numbers(p, customization, gd, grid);
        draw_available_numbers(p, customization, gd, grid);

        Point caret = {0, 0};
        bool is_game_over = false;
        bool input_registered_last_iteration = false;
        int selected_digit = 1;
        draw_circle_selector(p, gd, selected_digit,
                             customization->accent_color);

        while (!is_game_over) {
                Direction dir;
                Action act;
                if (poll_directional_input(p->directional_controllers, &dir)) {
                        erase_caret(p, gd, caret);
                        translate_toroidal_array(&caret, dir, SUDOKU_GRID_SIZE,
                                                 SUDOKU_GRID_SIZE);
                        draw_caret(p, customization, gd, caret);
                        // The delay below was hand-tweaked to feel
                        // good.
                        p->time_provider->delay_ms(GAME_LOOP_DELAY * 3 / 2);
                }
                if (poll_action_input(p->action_controllers, &act)) {
                        if (input_registered_last_iteration) {
                                continue;
                        }
                        input_registered_last_iteration = true;
                        switch (act) {
                        case GREEN: {
                                erase_circle_selector(p, gd, selected_digit);
                                selected_digit =
                                    selected_digit % SUDOKU_GRID_SIZE + 1;
                                draw_circle_selector(
                                    p, gd, selected_digit,
                                    customization->accent_color);
                                break;
                        }
                        case YELLOW: {
                                erase_circle_selector(p, gd, selected_digit);
                                selected_digit =
                                    mathematical_modulo(selected_digit - 2,
                                                        SUDOKU_GRID_SIZE) +
                                    1;
                                draw_circle_selector(
                                    p, gd, selected_digit,
                                    customization->accent_color);
                                break;
                        }
                        case RED: {
                                // We populate / erase the selected grid
                                // location using the currently selected
                                // digit.
                                auto &cell = grid[caret.y][caret.x];
                                if (cell.is_user_defined) {
                                        if (cell.value.has_value()) {
                                                cell.value = std::nullopt;
                                                erase_number(p, customization,
                                                             gd, caret);
                                                draw_caret(p, customization, gd,
                                                           caret);
                                        } else {
                                                cell.value = selected_digit,
                                                draw_number(p, customization,
                                                            gd, caret, cell);
                                        }
                                }
                                break;
                        }

                        case BLUE:
                                return UserAction::Exit;
                        }
                        // We wait slightly longer after an action is
                        // selected.
                        p->time_provider->delay_ms(2 * GAME_LOOP_DELAY);
                } else {
                        input_registered_last_iteration = false;
                }
                p->time_provider->delay_ms(CONTROL_POLLING_DELAY);
                p->display->refresh();
        }
        return UserAction::PlayAgain;
}

void draw_number(Platform *p, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location,
                 const SudokuCell &cell)
{

        assert(cell.value.has_value() &&
               "Only cells with values should be rendered");

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;
        int x_offset = location.x * cell_size;
        int y_offset = location.y * cell_size;

        // To ensure that the characters are centered inside of each
        // cell, we need to calculate the 'padding' around each
        // character. This is defined by the font height and width. Note
        // that we are subtracting one to take the cell border into
        // account and make it visually centered.
        int border_adjustment_offset = 1;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;

        int x_padding = (cell_size - fw) / 2 - border_adjustment_offset;
        int y_padding = (cell_size - fh) / 2 - border_adjustment_offset;

        char buffer[2];
        sprintf(buffer, "%d", cell.value.value());

        int x = x_margin + x_offset + x_padding;
        int y = y_margin + y_offset + y_padding;

        Color render_color = cell.is_user_defined ? White : Gray;
        p->display->draw_string({x, y}, buffer, FontSize::Size16, Black,
                                render_color);
}

void erase_number(Platform *p, UserInterfaceCustomization *customization,
                  SquareCellGridDimensions *dimensions, Point location)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / SUDOKU_GRID_SIZE;
        int x_offset = location.x * cell_size;
        int y_offset = location.y * cell_size;

        // To ensure that the characters are centered inside of each
        // cell, we need to calculate the 'padding' around each
        // character. This is defined by the font height and width. Note
        // that we are subtracting one to take the cell border into
        // account and make it visually centered.
        int border_adjustment_offset = 1;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;

        int x_padding = (cell_size - fw) / 2 - border_adjustment_offset;
        int y_padding = (cell_size - fh) / 2 - border_adjustment_offset;

        int x = x_margin + x_offset + x_padding;
        int y = y_margin + y_offset + y_padding;

        p->display->clear_region({x, y}, {x + fw, y + fh}, Black);
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
 * Forward declarations of functions related to configuration
 * manipulation.
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

        // We initialize empty config to detect corrupted memory and
        // fallback to defaults if needed.
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
