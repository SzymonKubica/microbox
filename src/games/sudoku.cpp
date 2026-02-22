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
#include "../common/maths_utils.hpp"
#include "sudoku_engine.hpp"

#define GAME_LOOP_DELAY 50
#define CONTROL_POLLING_DELAY 10

#define TAG "sudoku"

SudokuConfiguration DEFAULT_SUDOKU_CONFIG = {.difficulty = 1,
                                             .is_game_in_progress = false};

class SudokuState
{
        /**
         * Validates that a given point location can be used to index the 9x9
         * Sudoku grid.
         */
        void validate_grid_location(const Point &location)
        {
                assert(0 <= location.x && location.x <= 9);
                assert(0 <= location.y && location.y <= 9);
        }

        void decrement_digit_count(int digit)
        {
                int digit_idx = digit - 1;
                digits_placed--;
                digit_counts[digit_idx]--;
        }
        void increment_digit_count(int digit)
        {
                int digit_idx = digit - 1;
                digits_placed++;
                digit_counts[digit_idx]++;
        }

      public:
        /**
         * The digit that is currently selected by the user and will be placed
         * on the grid.
         */
        int active_digit = 1;
        /**
         * A counter maintaining the total number of digits placed on the grid.
         * This is required to know when to trigger the sudoku solution
         * validation logic (it is too wasteful to iterate over all rows,
         * columns and squares after each update to the grid).
         */
        int digits_placed = 0;
        /**
         * A 'map' from the digit index to the number of occurrences of this
         * digit on the grid. This is required for tracking when each digit is
         * 'done' and should be rendered differently in the available digits
         * indicator.
         */
        int digit_counts[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        SudokuGrid grid;

        SudokuState(SudokuGrid grid) : grid(grid)
        {
                for (int y = 0; y < 9; y++) {
                        for (int x = 0; x < 9; x++) {
                                auto &cell = grid[y][x];
                                if (cell.digit.has_value()) {
                                        this->increment_digit_count(
                                            cell.digit.value());
                                }
                        }
                }
        }

        /**
         * Given a point on the 9x9 grid, it erases the digit that was there and
         * updates the state counters after the removal. It returns the number
         * that was erased.
         */
        int erase_digit(const Point &location)
        {
                validate_grid_location(location);

                auto &cell = grid[location.y][location.x];

                assert(cell.is_user_defined &&
                       "Only user-defined cells can be erased.");
                assert(cell.digit.has_value() &&
                       "Only cells storing values can be erased.");

                int digit = cell.digit.value();
                cell.digit = std::nullopt;
                this->decrement_digit_count(digit);
                return digit;
        }

        /**
         * Given a point on the 9x9 grid, and a digit to be placed there, it
         * places the `active_digit` on the grid and updates the state counters
         * after the addition.
         */
        void place_digit(const Point &location)
        {
                validate_grid_location(location);

                auto &cell = grid[location.y][location.x];

                assert(cell.is_user_defined &&
                       "Only user-defined cells can be modified.");
                assert(!cell.digit.has_value() &&
                       "Only empty cells can be filled with numbers.");

                cell.digit = this->active_digit;
                this->increment_digit_count(this->active_digit);
        }

        inline bool is_grid_full() const
        {
                return this->digits_placed == 9 * 9;
        }

        inline bool is_complete(int digit) const
        {
                int digit_idx = digit - 1;
                return this->digit_counts[digit_idx] == 9;
        }
        inline int get_digit_count(int digit) const
        {
                int digit_idx = digit - 1;
                return this->digit_counts[digit_idx];
        }

        void increment_active_digit()
        {
                this->active_digit = this->active_digit % 9 + 1;
        }

        void decrement_active_digit()
        {
                this->active_digit =
                    mathematical_modulo(this->active_digit - 2, 9) + 1;
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

std::vector<std::vector<SudokuCell>>
load_game_state(SudokuConfiguration &config)
{
        std::vector<std::vector<SudokuCell>> grid(9,
                                                  std::vector(9, SudokuCell{}));
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        grid[y][x] = config.saved_game[y][x];
                }
        }
        return grid;
}

void save_game_state(Platform *p, SudokuConfiguration &config,
                     std::vector<std::vector<SudokuCell>> &grid)
{
        config.is_game_in_progress = true;

        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
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
                 const SudokuCell &value, int active_number,
                 std::optional<Color> color_override = std::nullopt);

void erase_number(Platform *p, UserInterfaceCustomization *customization,
                  SquareCellGridDimensions *dimensions, Point location);

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
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.digit.has_value()) {
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
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        const auto &cell = grid[y][x];
                        if (cell.digit.has_value() &&
                            digit_to_update == cell.digit.value()) {
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
        for (int i = 0; i < 9; i++) {
                SudokuCell cell(i + 1, true);
                // Here we repurpose the number drawing functionality to
                // render available numbers two cells to the left of the main
                // grid. We set the active number to 0 to ensure that the
                // indicator does not get rendered.
                draw_number(p, customization, dimensions, {-2, i}, cell, 0);
        }
}

/**
 * When the user places 9 occurences of a given digit we mark it as 'done' in
 * the selector list to the left of the sudoku grid. This is important to make
 * eliminating possible numbers easier.
 */
void mark_number_as_done(Platform *p, UserInterfaceCustomization *customization,
                         SquareCellGridDimensions *dimensions, int digit)
{
        int digit_idx = digit - 1;
        SudokuCell cell(digit, true);
        // Here we repurpose the number drawing functionality to
        // render available numbers two cells to the left of the main
        // grid. We set the active number to 0 to ensure that the
        // indicator does not get rendered.
        Point location = {-2, digit_idx};
        erase_number(p, customization, dimensions, location);
        draw_number(p, customization, dimensions, location, cell, 0,
                    customization->accent_color);
}

/**
 * This function reverses the highlight on the given digit in the selector if
 * there are no longer 9 occurences of this digit present on the grid.
 */
void unmark_number_as_done(Platform *p,
                           UserInterfaceCustomization *customization,
                           SquareCellGridDimensions *dimensions, int digit)
{
        int digit_idx = digit - 1;
        SudokuCell cell(digit, true);
        // Here we repurpose the number drawing functionality to
        // render available numbers two cells to the left of the main
        // grid. We set the active number to 0 to ensure that the
        // indicator does not get rendered.
        Point location = {-2, digit_idx};
        erase_number(p, customization, dimensions, location);
        draw_number(p, customization, dimensions, location, cell, 0);
}

void draw_circle_selector(Platform *p, SquareCellGridDimensions *dimensions,
                          int selected_number, Color color)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
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

        int cell_size = dimensions->actual_height / 9;
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
            p->display->get_display_corner_radius(), 9, 9, true);

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
                        grid = SudokuEngine::generate_grid(config.difficulty);
                }
        } else {
                grid = SudokuEngine::generate_grid(config.difficulty);
        }

        LOG_DEBUG(TAG, "Rendering sudoku game area.");
        SudokuState state = SudokuState(grid);
        draw_sudoku_grid_frame(p, customization, gd);
        draw_grid_numbers(p, customization, gd, state.grid, state.active_digit);

        draw_circle_selector(p, gd, state.active_digit,
                             customization->accent_color);
        draw_available_numbers(p, customization, gd, state.grid);

        Point caret = {0, 0};
        bool is_game_over = false;
        bool input_registered_last_iteration = false;

        while (!is_game_over) {
                if (state.is_grid_full()) {
                        if (SudokuEngine::validate(state.grid)) {
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
                        translate_toroidal_array(&caret, dir, 9, 9);
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
                                int old_selected = state.active_digit;
                                erase_circle_selector(p, gd,
                                                      state.active_digit);
                                state.increment_active_digit();
                                draw_circle_selector(
                                    p, gd, state.active_digit,
                                    customization->accent_color);

                                update_single_digit(
                                    p, customization, gd, state.grid,
                                    state.active_digit, old_selected);
                                update_single_digit(
                                    p, customization, gd, state.grid,
                                    state.active_digit, state.active_digit);

                                // If the caret was placed on any of the updated
                                // digits, it will get clipped, so we need to
                                // redraw it.
                                erase_caret(p, gd, caret);
                                draw_caret(p, customization, gd, caret);
                                break;
                        }
                        case YELLOW: {
                                int old_selected = state.active_digit;
                                erase_circle_selector(p, gd,
                                                      state.active_digit);
                                state.decrement_active_digit();
                                draw_circle_selector(
                                    p, gd, state.active_digit,
                                    customization->accent_color);

                                update_single_digit(
                                    p, customization, gd, state.grid,
                                    state.active_digit, old_selected);
                                update_single_digit(
                                    p, customization, gd, state.grid,
                                    state.active_digit, state.active_digit);

                                // If the caret was placed on any of the updated
                                // digits, it will get clipped, so we need to
                                // redraw it.
                                erase_caret(p, gd, caret);
                                draw_caret(p, customization, gd, caret);
                                break;
                        }
                        case RED: {
                                auto &cell = state.grid[caret.y][caret.x];
                                if (cell.is_user_defined &&
                                    cell.digit.has_value()) {
                                        int erased = state.erase_digit(caret);
                                        int count =
                                            state.get_digit_count(erased);
                                        // If the count is 8 after
                                        // removal it means that the
                                        // digit had 9 occurrences
                                        // before the removal and is now
                                        // no longer complete.
                                        if (count == 8)
                                                unmark_number_as_done(
                                                    p, customization, gd,
                                                    erased);
                                        // View update
                                        erase_number(p, customization, gd,
                                                     caret);
                                        draw_caret(p, customization, gd, caret);
                                } else if (cell.is_user_defined) {
                                        state.place_digit(caret);
                                        if (state.is_complete(
                                                state.active_digit)) {
                                                LOG_DEBUG(TAG, "Digit %d done.",
                                                          state.active_digit);
                                                mark_number_as_done(
                                                    p, customization, gd,
                                                    state.active_digit);
                                        }
                                        // View update
                                        draw_number(p, customization, gd, caret,
                                                    cell, state.active_digit);
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
                                        save_game_state(p, config, state.grid);
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
                if (!p->display->refresh()) {
                        delete gd;
                        return UserAction::CloseWindow;
                }
        }
        return UserAction::PlayAgain;
}

void draw_number(Platform *p, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location,
                 const SudokuCell &cell, int active_number,
                 std::optional<Color> color_override)
{

        assert(cell.digit.has_value() &&
               "Only cells with values should be rendered");

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
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
        sprintf(buffer, "%d", cell.digit.value());

        // Because of pixel-precision inaccuracies we need to adjust the
        // placement of the numbers in the grid.
#ifdef EMULATOR
        int adj = 0;
#else
        int adj = 1;
#endif
        int x = x_margin + x_offset + x_padding + adj;
        int y = y_margin + y_offset + y_padding + adj;

        Color render_color = cell.is_user_defined ? White : Gray;

        if (color_override.has_value()) {
                render_color = color_override.value();
        }

        p->display->draw_string({x, y}, buffer, FontSize::Size16, Black,
                                render_color);
        if (cell.digit.value() == active_number) {
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

        int cell_size = dimensions->actual_height / 9;
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

        // Because of pixel-precision inaccuracies we need to adjust the
        // placement of the numbers in the grid.
#ifdef EMULATOR
        int adj = 0;
#else
        int adj = 1;
#endif
        int x = x_margin + x_offset + x_padding + adj;
        int y = y_margin + y_offset + y_padding + adj;

        // For active numbers that were underlined we need to erase a bit
        // further down to remove the underline.
        int underline_adj = 1;

        p->display->clear_region({x, y}, {x + fw + adj, y + fh + underline_adj},
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

        int cell_size = dimensions->actual_height / 9;

        // We only render borders between cells for a minimalistic look
        for (int i = 1; i < 9; i++) {
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
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        game_config->saved_game[y][x] =
                            initial_config->saved_game[y][x];
                }
        }
}

/**
 * Graveyard below (TODO: clean-up / repurpose)
 */
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
        for (int i = 0; i < 9; i++) {
                std::vector<SudokuCell> row;
                for (int j = 0; j < 9; j++) {
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
