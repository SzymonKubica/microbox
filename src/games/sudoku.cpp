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
#include "sudoku_view.hpp"

#define GAME_LOOP_DELAY 50
#define CONTROL_POLLING_DELAY 10

#define TAG "sudoku"

SudokuConfiguration DEFAULT_SUDOKU_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1},
    .difficulty = 1,
    .is_game_in_progress = false,
    .accent_color = Color::Cyan};

/**
 * Represents the state of a Sudoku game.
 */
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

const char *SudokuGame::get_game_name() { return "Sudoku"; }
const char *SudokuGame::get_help_text()
{
        return "Use the left/right buttons to select which digit you are "
               "inserting. Use the joystick to control the cursor. Press green "
               "to "
               "(re-)place the current digit.";
}

std::vector<std::vector<SudokuCell>>
load_game_state(const SudokuConfiguration &config)
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

UserAction SudokuGame::app_loop(Platform *p,
                                UserInterfaceCustomization *customization,
                                const SudokuConfiguration &config)
{
        LOG_DEBUG(TAG, "Entering sudoku game loop");

        SquareCellGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), 9, 9, true);

        SimpleSudokuView view{*customization, *gd, p->display};

        std::vector<std::vector<SudokuCell>> grid;
        if (config.is_game_in_progress) {
                const char *help_text =
                    "A game in progress was found. Press green to "
                    "continue the previous game or red to start a "
                    "new game.";
                render_wrapped_text(p, customization, help_text);
                Action action;
                auto maybe_interrupt = wait_until_action_input(p, &action);
                if (maybe_interrupt.has_value()) {
                        assert(maybe_interrupt.value() ==
                               UserAction::CloseWindow);
                        return UserAction::CloseWindow;
                }
                if (action == Action::GREEN) {
                        grid = load_game_state(config);
                } else {
                        grid = SudokuEngine::generate_grid(config.difficulty);
                        LOG_DEBUG(TAG, "Grid generated successfully.");
                }
        } else {
                grid = SudokuEngine::generate_grid(config.difficulty);
                LOG_DEBUG(TAG, "Grid generated successfully.");
        }

        LOG_DEBUG(TAG, "Initializing game state.");
        SudokuState state(grid);
        LOG_DEBUG(TAG, "Rendering sudoku game area.");

        view.render_grid();
        view.render_grid_numbers(state.grid);
        view.underline_all_instances(state.active_digit, state.grid,
                                     config.accent_color);
        view.render_active_digit_selector();
        view.render_active_digit_selection_indicator(state.active_digit);
        // If we resumed a game in progress and loaded the grid from the
        // storage, we need to re-highlight all completed numbers.
        for (int digit = 1; digit <= 9; digit++) {
                if (state.is_complete(digit))
                        view.mark_digit_completed(digit);
        }

        Point caret = {0, 0};
        /*
         * To avoid button debounce issues we keep track of whether the input
         * was also registered on the previous iteration. If that is the case,
         * we short-circuit processing. This needs to be done to avoid issues
         * where a slow press of the physical arduino button causes a
         * double-click and e.g. sudoku cell values start flickering between
         * filled and empty.
         */
        bool input_registered_last_iteration = false;

        while (true) {
                if (!p->display->refresh()) {
                        delete gd;
                        return UserAction::CloseWindow;
                }
                if (state.is_grid_full() &&
                    SudokuEngine::validate(state.grid)) {
                        auto help_text = "Congratulations, you solved "
                                         "the sudoku successfully!";
                        render_wrapped_help_text(p, customization, help_text);
                        auto maybe_interrupt = wait_until_green_pressed(p);

                        if (maybe_interrupt.has_value())
                                return maybe_interrupt.value();
                        return UserAction::Exit;
                }
                Direction dir;
                Action act;
                if (poll_directional_input(p->directional_controllers, &dir)) {
                        Point previous = caret;
                        translate_toroidal_array(&caret, dir, 9, 9);
                        view.move_caret(previous, caret);
                        // The delay below was hand-tweaked to feel
                        // good.
                        p->time_provider->delay_ms(GAME_LOOP_DELAY * 3 / 2);
                }
                if (!poll_action_input(p->action_controllers, &act)) {
                        input_registered_last_iteration = false;
                        p->time_provider->delay_ms(CONTROL_POLLING_DELAY);
                        continue;
                }
                // If the user holds the button depressed, we still only act
                // once to avoid double-processing of slow presses caused by
                // button debounce issues.
                if (input_registered_last_iteration) {
                        p->time_provider->delay_ms(CONTROL_POLLING_DELAY);
                        continue;
                }
                // Before processing the input we record that we handled it on
                // this iteration.
                input_registered_last_iteration = true;
                switch (act) {
                case YELLOW:
                case GREEN: {
                        int old_selected = state.active_digit;

                        act == GREEN ? state.increment_active_digit()
                                     : state.decrement_active_digit();

                        view.move_active_digit_selection_indicator(
                            old_selected, state.active_digit);
                        view.remove_underline_all_instances(old_selected,
                                                            state.grid);
                        view.underline_all_instances(state.active_digit,
                                                     state.grid,
                                                     config.accent_color);

                        // If the caret was placed on any of the updated digits,
                        // it will get clipped, so we need to redraw it.
                        view.rerender_caret(caret);
                        break;
                }
                case RED: {
                        auto &cell = state.grid[caret.y][caret.x];
                        if (cell.is_user_defined && cell.digit.has_value()) {
                                int erased = state.erase_digit(caret);
                                int count = state.get_digit_count(erased);
                                // If the count is 8 after removal it means that
                                // the digit had 9 occurrences before the
                                // removal and is now no longer complete.
                                if (count == 8)
                                        view.unmark_digit_completed(erased);
                                view.erase_cell_contents(caret);
                                view.render_caret(caret);
                        } else if (cell.is_user_defined) {
                                state.place_digit(caret);
                                if (state.is_complete(state.active_digit)) {
                                        LOG_DEBUG(TAG, "Digit %d done.",
                                                  state.active_digit);
                                        view.mark_digit_completed(
                                            state.active_digit);
                                }
                                view.render_cell(cell, caret);
                                // Only the active digit can be placed and all
                                // occurrences of the active digit need to be
                                // underlined so we place the underline here.
                                view.underline_cell(caret, config.accent_color);
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
                        render_wrapped_text(p, customization, help_text);
                        Action action;
                        auto maybe_interrupt =
                            wait_until_action_input(p, &action);
                        if (maybe_interrupt.has_value()) {
                                assert(maybe_interrupt.value() ==
                                       UserAction::CloseWindow);
                                return UserAction::CloseWindow;
                        }
                        if (action == Action::GREEN) {
                                SudokuConfiguration copy = config;
                                save_game_state(p, copy, state.grid);
                        }
                        p->time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                        delete gd;
                        return UserAction::Exit;
                }
                }
                // We wait slightly longer after an action is
                // selected.
                p->time_provider->delay_ms(2 * GAME_LOOP_DELAY);
        }
        return UserAction::PlayAgain;
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
SudokuGame::collect_config(Platform *p,
                           UserInterfaceCustomization *customization,
                           SudokuConfiguration *game_config)
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

        ConfigurationOption *accent_color = ConfigurationOption::of_colors(
            "Color", AVAILABLE_COLORS, initial_config->accent_color);

        std::vector<ConfigurationOption *> options = {difficulty, accent_color};

        return new Configuration("Sudoku", options);
}

SudokuConfiguration *load_initial_sudoku_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::Sudoku);
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        SudokuConfiguration config{};

        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage->get(storage_offset, config);

        SudokuConfiguration *output = new SudokuConfiguration();

        if (!config.header.is_valid()) {
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
        ConfigurationOption accent_color = *config->options[1];

        game_config->difficulty = difficulty.get_curr_int_value();
        game_config->is_game_in_progress = initial_config->is_game_in_progress;
        game_config->accent_color = accent_color.get_current_color_value();
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
