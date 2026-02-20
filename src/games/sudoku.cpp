#include <cassert>
#include <memory>
#include <optional>
#include <algorithm>
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

#define TAG "sudoku"

SudokuConfiguration DEFAULT_SUDOKU_CONFIG = {.difficulty = 1,
                                             .is_game_in_progress = false};

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

std::vector<std::vector<SudokuCell>>
load_game_state(SudokuConfiguration &config)
{
        std::vector<std::vector<SudokuCell>> grid(
            SUDOKU_GRID_SIZE, std::vector(SUDOKU_GRID_SIZE, SudokuCell{}));
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        grid[y][x] = config.saved_game[y][x];
                }
        }
        return grid;
}

void save_game_state(Platform *p, SudokuConfiguration &config,
                     std::vector<std::vector<SudokuCell>> &grid)
{
        config.is_game_in_progress = true;

        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        config.saved_game[y][x] = grid[y][x];
                }
        }

        int storage_offset = get_settings_storage_offset(Game::Sudoku);
        LOG_DEBUG(TAG,
                  "Saving current Sudoku game state to persistent storage at "
                  "offset %d",
                  storage_offset);
        p->persistent_storage->put(storage_offset, config);
}

void draw_sudoku_grid_frame(Platform *p,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions);

void draw_number(Platform *p, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location,
                 const SudokuCell &value, int active_number);

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

std::vector<std::vector<SudokuCell>>
generate_solvable_grid(SudokuConfiguration &config)
{
        auto solvable = generate_solved_grid();

        // According to the online wisdom, a sudoku with ~40 cells left
        // tends to be easy. If you leave ~30 it becomes medium and if only
        // ~20 cells are left it turns out to be hard. Because of this, we
        // calculate the number of digits to remove as:
        // 30 + 10 * x where x is the difficulty level (1, 2, or 3).
        // This gives us: 1 -> 40, 2 -> 50 and 3 -> 60 which is what we want.
        int to_remove = 30 + 10 * config.difficulty;

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

/**
 * Active number is the one that the user has currently selected and will be
 * placing on the grid. We render this number using the accent color to
 * make it visually stand out.
 */
void draw_grid_numbers(Platform *p, UserInterfaceCustomization *customization,
                       SquareCellGridDimensions *dimensions,
                       const std::vector<std::vector<SudokuCell>> &grid,
                       int active_number)
{
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.value.has_value()) {
                                draw_number(p, customization, dimensions,
                                            {x, y}, cell, active_number);
                        }
                }
        }
}

void update_single_digit(Platform *p, UserInterfaceCustomization *customization,
                         SquareCellGridDimensions *dimensions,
                         const std::vector<std::vector<SudokuCell>> &grid,
                         int active_number, int digit_to_update)
{
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.value.has_value() &&
                            digit_to_update == cell.value.value()) {
                                erase_number(p, customization, dimensions,
                                             {x, y});
                                draw_number(p, customization, dimensions,
                                            {x, y}, cell, active_number);
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
                // Here we repurpose the number drawing functionality to
                // render available numbers two cells to the left of the main
                // grid. We set the active number to 0 to ensure that the
                // indicator does not get rendered.
                draw_number(p, customization, dimensions, {-2, i}, cell, 0);
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
#ifdef EMULATOR
        int y_adjustment = 3;
#else
        int y_adjustment = 1;
#endif

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

bool validate_grid(std::vector<std::vector<SudokuCell>> &grid)
{

        // Check rows
        for (int y = 0; y < 9; y++) {
                int digit_counts[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
                LOG_DEBUG(TAG, "Validating row %d", y);
                for (int x = 0; x < 9; x++) {
                        auto &cell = grid[y][x];
                        if (!cell.value.has_value())
                                return false;
                        int value = cell.value.value();
                        digit_counts[value - 1]++;
                }
                for (int i = 0; i < 9; i++) {
                        int digit = i + 1;
                        int count = digit_counts[i];
                        if (count != 1) {
                                LOG_DEBUG(TAG,
                                          "Digit %d was found %d times. This "
                                          "is incorrect",
                                          digit, count);
                                return false;
                        }
                }
        }
        LOG_DEBUG(TAG, "Rows validated successfully!");

        // Check columns
        for (int x = 0; x < 9; x++) {
                int digit_counts[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
                for (int y = 0; y < 9; y++) {
                        auto &cell = grid[y][x];
                        if (!cell.value.has_value())
                                return false;
                        int value = cell.value.value();
                        digit_counts[value - 1]++;
                }
                for (int count : digit_counts) {
                        if (count != 1) {
                                return false;
                        }
                }
        }
        LOG_DEBUG(TAG, "Columns validated successfully!");

        for (int y = 0; y < 9; y += 3) {
                for (int x = 0; x < 9; x += 3) {
                        // Check big square
                        Point square_top_left = {x, y};

                        int digit_counts[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
                        for (int y = square_top_left.y;
                             y < square_top_left.y + 3; y++) {
                                for (int x = square_top_left.x;
                                     x < square_top_left.x + 3; x++) {

                                        auto &cell = grid[y][x];
                                        if (!cell.value.has_value())
                                                return false;
                                        int value = cell.value.value();
                                        digit_counts[value - 1]++;
                                }
                        }

                        for (int count : digit_counts) {
                                if (count != 1) {
                                        return false;
                                }
                        }
                }
        }
        LOG_DEBUG(TAG, "Squares validated successfully!");
        return true;
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

        std::vector<std::vector<SudokuCell>> grid;
        if (config.is_game_in_progress) {
                const char *help_text =
                    "A game in progress was found. Press green to "
                    "continue the previous game or red to start a "
                    "new game.";
                render_wrapped_text(p, customization, help_text);
                auto action = wait_until_action_input(p);
                if (std::holds_alternative<UserAction>(action)) {
                        assert(std::get<UserAction>(action) ==
                               UserAction::CloseWindow);
                        return UserAction::CloseWindow;
                }
                if (std::get<Action>(action) == Action::GREEN) {
                        grid = load_game_state(config);
                } else {
                        grid = generate_solvable_grid(config);
                }
        } else {
                grid = generate_solvable_grid(config);
        }

        LOG_DEBUG(TAG, "Rendering sudoku game area.");
        draw_sudoku_grid_frame(p, customization, gd);
        int selected_digit = 1;
        draw_circle_selector(p, gd, selected_digit,
                             customization->accent_color);
        draw_available_numbers(p, customization, gd, grid);
        draw_grid_numbers(p, customization, gd, grid, selected_digit);

        Point caret = {0, 0};
        bool is_game_over = false;
        bool input_registered_last_iteration = false;

        int numbers_placed = 0;

        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        if (!grid[y][x].is_user_defined) {
                                numbers_placed++;
                        }
                }
        }

        while (!is_game_over) {
                if (numbers_placed == SUDOKU_GRID_SIZE * SUDOKU_GRID_SIZE) {
                        if (validate_grid(grid)) {
                                auto help_text = "Congratulations, you solved "
                                                 "the sudoku successfully!";
                                render_wrapped_help_text(p, customization,
                                                         help_text);
                                maybe_interrupt = wait_until_green_pressed(p);

                                if (maybe_interrupt.has_value()) {
                                        return maybe_interrupt.value();
                                }
                                return UserAction::Exit;
                        }
                }
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

                                int old_selected = selected_digit;
                                erase_circle_selector(p, gd, selected_digit);
                                selected_digit =
                                    selected_digit % SUDOKU_GRID_SIZE + 1;
                                draw_circle_selector(
                                    p, gd, selected_digit,
                                    customization->accent_color);

                                update_single_digit(p, customization, gd, grid,
                                                    selected_digit,
                                                    old_selected);
                                update_single_digit(p, customization, gd, grid,
                                                    selected_digit,
                                                    selected_digit);

                                // If the caret was placed on any of the updated
                                // digits, it will get clipped, so we need to
                                // redraw it.
                                erase_caret(p, gd, caret);
                                draw_caret(p, customization, gd, caret);
                                break;
                        }
                        case YELLOW: {
                                int old_selected = selected_digit;
                                erase_circle_selector(p, gd, selected_digit);
                                selected_digit =
                                    mathematical_modulo(selected_digit - 2,
                                                        SUDOKU_GRID_SIZE) +
                                    1;
                                draw_circle_selector(
                                    p, gd, selected_digit,
                                    customization->accent_color);

                                update_single_digit(p, customization, gd, grid,
                                                    selected_digit,
                                                    old_selected);
                                update_single_digit(p, customization, gd, grid,
                                                    selected_digit,
                                                    selected_digit);

                                // If the caret was placed on any of the updated
                                // digits, it will get clipped, so we need to
                                // redraw it.
                                erase_caret(p, gd, caret);
                                draw_caret(p, customization, gd, caret);
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
                                                numbers_placed--;
                                        } else {
                                                cell.value = selected_digit,
                                                draw_number(p, customization,
                                                            gd, caret, cell,
                                                            selected_digit);
                                                numbers_placed++;
                                        }
                                }
                                break;
                        }

                        case BLUE: {
                                LOG_DEBUG(TAG, "User requested to exit game.");
                                const char *help_text =
                                    "Would you like to save your current game "
                                    "state and resume it later? Press green to "
                                    "save and exit, or blue to exit without "
                                    "saving.";
                                render_wrapped_text(p, customization,
                                                    help_text);
                                auto action = wait_until_action_input(p);
                                if (std::holds_alternative<UserAction>(
                                        action)) {
                                        assert(std::get<UserAction>(action) ==
                                               UserAction::CloseWindow);
                                        return UserAction::CloseWindow;
                                }
                                if (std::get<Action>(action) == Action::GREEN) {
                                        save_game_state(p, config, grid);
                                }
                                p->time_provider->delay_ms(
                                    MOVE_REGISTERED_DELAY);
                                delete gd;
                                return UserAction::Exit;
                        }
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
                 const SudokuCell &cell, int active_number)
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
        if (cell.value.value() == active_number) {
                // We underline the active number to make it pop visually.
                p->display->draw_line({x, y + fh - 1}, {x + fw, y + fh - 1},
                                      customization->accent_color);
                p->display->draw_line({x, y + fh}, {x + fw, y + fh},
                                      customization->accent_color);
        }
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

        // For active numbers that were underlined we need to erase a bit
        // further down to remove the underline.
        int underline_adj = 1;

        p->display->clear_region({x, y}, {x + fw, y + fh + underline_adj},
                                 Black);
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
Configuration *
assemble_sudoku_configuration(SudokuConfiguration *initial_config);
void extract_game_config(SudokuConfiguration *game_config,
                         Configuration *config,
                         SudokuConfiguration *initial_config);

std::optional<UserAction>
collect_sudoku_config(Platform *p, SudokuConfiguration *game_config,
                      UserInterfaceCustomization *customization)
{
        SudokuConfiguration *initial_config =
            load_initial_sudoku_config(p->persistent_storage);

        Configuration *config = assemble_sudoku_configuration(initial_config);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                delete initial_config;
                delete config;
                return maybe_interrupt;
        }

        extract_game_config(game_config, config, initial_config);
        delete initial_config;
        delete config;
        return std::nullopt;
}

Configuration *
assemble_sudoku_configuration(SudokuConfiguration *initial_config)
{

        ConfigurationOption *difficulty = ConfigurationOption::of_integers(
            "Difficulty", {1, 2, 3}, initial_config->difficulty);

        std::vector<ConfigurationOption *> options = {difficulty};

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

        LOG_DEBUG(TAG,
                  "Loaded sudoku configuration: difficulty=%d, "
                  "is_game_in_progress=%d",
                  output->difficulty, output->is_game_in_progress);

        return output;
}

void extract_game_config(SudokuConfiguration *game_config,
                         Configuration *config,
                         SudokuConfiguration *initial_config)
{
        ConfigurationOption difficulty = *config->options[0];

        game_config->difficulty = difficulty.get_curr_int_value();
        game_config->is_game_in_progress = initial_config->is_game_in_progress;
        for (int y = 0; y < SUDOKU_GRID_SIZE; y++) {
                for (int x = 0; x < SUDOKU_GRID_SIZE; x++) {
                        game_config->saved_game[y][x] =
                            initial_config->saved_game[y][x];
                }
        }
}
