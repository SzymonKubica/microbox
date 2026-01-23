#include <cstdint>
#include <cstring>

#include "../common/logging.hpp"
#include "../common/constants.hpp"
#include "../common/maths_utils.hpp"
#include "game_of_life.hpp"
#include "settings.hpp"
#include "game_menu.hpp"

#define TAG "game_of_life"
#define GAME_CELL_WIDTH 8

#define GAME_LOOP_DELAY 100

#ifdef EMULATOR
#define EXPLANATION_ABOVE_GRID_OFFEST 4
#else
#define EXPLANATION_ABOVE_GRID_OFFEST 0
#endif

#define REWIND_BUF_SIZE 50

#define ALIVE true
#define EMPTY false

GameOfLifeConfiguration DEFAULT_GAME_OF_LIFE_CONFIG = {
    .prepopulate_grid = false,
    .use_toroidal_array = true,
    .simulation_speed = 2,
    .rewind_buffer_size = REWIND_BUF_SIZE,
};

typedef bool GameOfLifeCell;
typedef uint8_t *Grid;
inline bool get_cell(int x, int y, int cols, Grid grid);
inline void set_cell(int x, int y, int cols, Grid grid, bool alive);
inline Grid allocate_grid(int cells);

/**
 * Models a change of the Game of Life state from one frame to another. This
 * is needed to render changes when a single iteration of the simulation loop
 * is taken. Note that counterintuitively, it is more efficient to crate a new
 * full grid (represented as a bitset) on each iteration than it is to create
 * diff objects. This is beause our grid is relatively small.
 */
typedef std::pair<uint8_t *, uint8_t *> StateEvolution;

typedef enum SimulationMode {
        RUNNING = 0,
        PAUSED = 1,
        REWIND = 2,
} SimulationMode;

/**
 * Assembles the generic configuration struct that can be used to collect user
 * input specifying the game of life configuration.
 */
Configuration *assemble_game_of_life_configuration(PersistentStorage *storage);

/**
 * Extracts the specific game of life config struct after the generic config
 * was collected from the user.
 */
void extract_game_config(GameOfLifeConfiguration *game_config,
                         Configuration *config);

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius);

void draw_rewind_mode_indicator(Platform *p,
                                SquareCellGridDimensions *dimensions,
                                UserInterfaceCustomization *customization);
void clear_rewind_mode_indicator(Platform *p,
                                 SquareCellGridDimensions *dimensions,
                                 UserInterfaceCustomization *customization);
void draw_caret(Display *display, Point *grid_position,
                SquareCellGridDimensions *dimensions, Color caret_color);
void erase_caret(Display *display, Point *grid_position,
                 SquareCellGridDimensions *dimensions,
                 Color grid_background_color);
void draw_game_cell(Display *display, Point *grid_position,
                    SquareCellGridDimensions *dimensions, Color color);

StateEvolution take_simulation_step(Grid grid,
                                    SquareCellGridDimensions *dimensions,
                                    bool use_toroidal_array);

void render_state_change(Display *display, StateEvolution evolution,
                         SquareCellGridDimensions *dimensions);

void spawn_cells_randomly(Display *display, Grid grid,
                          SquareCellGridDimensions *dimensions);

void save_grid_state_in_rewind_buffer(std::vector<Grid> *rewind_buffer,
                                      int *rewind_buf_idx, Grid grid);

Grid handle_rewind(Direction dir, std::vector<Grid> *rewind_buffer,
                   int latest_state_idx, int *rewind_buf_idx, Grid grid,
                   SquareCellGridDimensions *gd, Display *display);

const char *map_boolean_to_yes_or_no(bool value);

GameOfLifeConfiguration *
load_initial_game_of_life_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::GameOfLife);

        GameOfLifeConfiguration config = {.prepopulate_grid = false,
                                          .use_toroidal_array = false,
                                          .simulation_speed = 0,
                                          .rewind_buffer_size = 0};

        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        GameOfLifeConfiguration *output = new GameOfLifeConfiguration();

        if (config.rewind_buffer_size == 0) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "game of life configuration, using default values.");
                memcpy(output, &DEFAULT_GAME_OF_LIFE_CONFIG,
                       sizeof(GameOfLifeConfiguration));
                storage->put(storage_offset, DEFAULT_GAME_OF_LIFE_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(GameOfLifeConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded game of life configuration: prepopulate_grid=%d, "
                  "use_toroidal_array=%d, simulation_speed=%d, "
                  "rewind_buffer_size=%d",
                  output->prepopulate_grid, output->use_toroidal_array,
                  output->simulation_speed, output->rewind_buffer_size);

        return output;
}
/**
 * Returns true if the user wants to play again. If they press blue on the
 * configuration screen it means that they want to exit, in which case this
 * function would return false.
 */
UserAction game_of_life_loop(Platform *platform,
                             UserInterfaceCustomization *customization);

void GameOfLife::game_loop(Platform *p,
                           UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Use the joystick to move the caret around the grid. Press green "
            "to toggle the cell between alive/dead, yellow to pause, blue to "
            "rewind back in time, red to exit. There is no aim, you stare at "
            "the simulation";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (game_of_life_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_DEBUG(TAG, "Re-entering game of life loop.");
                        break;
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_DEBUG(TAG,
                                  "User requested game of life help screen.");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                }
        }
}

void render_help_hints(Display *display, SquareCellGridDimensions *dimensions,
                       int border_offset);
UserAction game_of_life_loop(Platform *p,
                             UserInterfaceCustomization *customization)
{

        LOG_DEBUG(TAG, "Entering Game of Life game loop");
        GameOfLifeConfiguration config;

        auto maybe_interrupt =
            collect_game_of_life_config(p, &config, customization);
        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        SquareCellGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), GAME_CELL_WIDTH);
        int rows = gd->rows;
        int cols = gd->cols;
        int total_cells = rows * cols;

        draw_grid_frame(p, customization, gd);
        LOG_DEBUG(TAG, "Game of Life canvas drawn.");

        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially). Note: this constant is duplicated inside of
        // draw_game_canvas and was moved out of the function so that that other
        // function can be reused for snake. Ideally we need to separate the two
        // concepts in a clean way
        int border_offset = 2;
        if (customization->show_help_text) {
                render_help_hints(p->display, gd, border_offset);
        }

        Point caret_pos = {.x = 0, .y = 0};
        draw_caret(p->display, &caret_pos, gd, customization->accent_color);

        /* Because of memory constraints, we need to use a hand-rolled bitset
           to store each 'frame' of the game simulation. The reason for this
           is that storing the diffs with two integer (x, y) coordinates
           occupies too much memory. */

        uint8_t *grid = allocate_grid(total_cells);

        /* A ring buffer storing previous simulation states that are used to
           allow for going back in time. Note that each user input also counts
           as a simulation step so will be included in the rewind buffer. */
        std::vector<uint8_t *> rewind_buffer(config.rewind_buffer_size,
                                             nullptr);

        int rewind_buf_idx = -1;
        // Keeps track of the latest diff in the rewind buffer. Prevents us from
        // wrapping back to it.
        int rewind_initial_idx = -1;

        if (config.prepopulate_grid) {
                spawn_cells_randomly(p->display, grid, gd);
        }

        int evolution_period =
            (1000 / config.simulation_speed) / GAME_LOOP_DELAY;
        int iteration = 0;

        bool exit_requested = false;
        SimulationMode mode = PAUSED;
        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'spawn' button for too long and
        // the cell flickers instead of getting spawned properly. We implement
        // this using this flag.
        bool action_input_on_last_iteration = false;
        while (!exit_requested) {
                if (mode == RUNNING && iteration == evolution_period - 1) {
                        LOG_DEBUG(TAG, "Taking a simulation step");
                        StateEvolution evolution = take_simulation_step(
                            grid, gd, config.use_toroidal_array);

                        render_state_change(p->display, evolution, gd);
                        save_grid_state_in_rewind_buffer(&rewind_buffer,
                                                         &rewind_buf_idx, grid);
                        grid = evolution.second;
                }
                Direction dir;
                Action act;
                GameOfLifeCell curr =
                    get_cell(caret_pos.x, caret_pos.y, gd->cols, grid);
                if (poll_directional_input(p->directional_controllers,
                                                 &dir)) {
                        if (mode == REWIND) {
                                grid = handle_rewind(
                                    dir, &rewind_buffer, rewind_initial_idx,
                                    &rewind_buf_idx, grid, gd, p->display);
                        } else {
                                Color bg_color =
                                    (curr == EMPTY) ? Black : White;
                                erase_caret(p->display, &caret_pos, gd,
                                            bg_color);

                                // Move the caret according to the user input.
                                auto translate = (config.use_toroidal_array)
                                                     ? translate_toroidal_array
                                                     : translate_within_bounds;
                                translate(&caret_pos, dir, gd->rows, gd->cols);
                                draw_caret(p->display, &caret_pos, gd,
                                           customization->accent_color);
                        }
                }
                if (poll_action_input(p->action_controllers, &act) &&
                    !action_input_on_last_iteration) {
                        action_input_on_last_iteration = true;
                        switch (act) {
                        case YELLOW:
                                if (mode == PAUSED) {
                                        mode = RUNNING;
                                        LOG_DEBUG(TAG, "Simulation running...");
                                } else if (mode == REWIND) {
                                        mode = PAUSED;
                                        LOG_DEBUG(
                                            TAG,
                                            "Simulation paused after rewind.");
                                        clear_rewind_mode_indicator(
                                            p, gd, customization);
                                } else {
                                        mode = PAUSED;
                                        LOG_DEBUG(TAG, "Simulation paused.");
                                }
                                break;
                        case RED:
                                exit_requested = true;
                                break;
                        case BLUE:
                                if (mode == REWIND) {
                                        mode = RUNNING;
                                        clear_rewind_mode_indicator(
                                            p, gd, customization);
                                        LOG_DEBUG(TAG, "Simulation running...");
                                } else if (rewind_buf_idx != -1) {
                                        // We can only rewind if the buffer has
                                        // at least one entry.
                                        mode = REWIND;
                                        draw_rewind_mode_indicator(
                                            p, gd, customization);
                                        // We need to record the latest index in
                                        // the rewind buffer.
                                        rewind_initial_idx = rewind_buf_idx;
                                        LOG_DEBUG(TAG, "Rewind mode enabled.");
                                }
                                break;
                        case GREEN:
                                Color new_cell_color;

                                // We copy the current state and only modify the
                                // caret position.
                                Grid new_grid = allocate_grid(total_cells);
                                int size = (total_cells + 7) / 8;
                                std::memcpy(new_grid, grid,
                                            size * sizeof(uint8_t));

                                if (curr == EMPTY) {
                                        set_cell(caret_pos.x, caret_pos.y, cols,
                                                 new_grid, ALIVE);
                                        new_cell_color = White;
                                } else if (curr == ALIVE) {
                                        set_cell(caret_pos.x, caret_pos.y, cols,
                                                 new_grid, EMPTY);
                                        new_cell_color = Black;
                                }

                                save_grid_state_in_rewind_buffer(
                                    &rewind_buffer, &rewind_buf_idx, grid);
                                draw_game_cell(p->display, &caret_pos, gd,
                                               new_cell_color);
                                // we need to redraw the caret as we have just
                                // drawn a cell by clearing the region
                                draw_caret(p->display, &caret_pos, gd,
                                           customization->accent_color);

                                grid = new_grid;
                                break;
                        }
                } else {
                        action_input_on_last_iteration = false;
                }
                iteration += 1;
                iteration %= evolution_period;
                p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                p->display->refresh();
        }
        return UserAction::PlayAgain;
}

std::optional<UserAction>
collect_game_of_life_config(Platform *p, GameOfLifeConfiguration *game_config,
                            UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_game_of_life_configuration(p->persistent_storage);
        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        free_configuration(config);
        return std::nullopt;
}

Configuration *assemble_game_of_life_configuration(PersistentStorage *storage)
{
        GameOfLifeConfiguration *initial_config =
            load_initial_game_of_life_config(storage);

        // Controls if grid will get pre-populated with cells randomly.
        auto *spawn_randomly = ConfigurationOption::of_strings(
            "Spawn randomly", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->prepopulate_grid));

        auto *simulation_speed = ConfigurationOption::of_integers(
            "Evolutions/second", {1, 2, 4}, initial_config->simulation_speed);

        // Controls if the grid is toroidal i.e. the edges wrap around.
        auto *toroidal_array = ConfigurationOption::of_strings(
            "Toroidal array", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->use_toroidal_array));

        free(initial_config);

        auto options = {spawn_randomly, simulation_speed, toroidal_array};

        return new Configuration("Game of Life", options);
}

void extract_game_config(GameOfLifeConfiguration *game_config,
                         Configuration *config)
{

        ConfigurationOption prepopulate_grid = *config->options[0];
        int curr_choice_idx = prepopulate_grid.currently_selected;
        const char *choice = static_cast<const char **>(
            prepopulate_grid.available_values)[curr_choice_idx];
        game_config->prepopulate_grid = extract_yes_or_no_option(choice);

        game_config->rewind_buffer_size = REWIND_BUF_SIZE;

        ConfigurationOption simulation_speed = *config->options[1];
        int curr_speed_idx = simulation_speed.currently_selected;
        game_config->simulation_speed = static_cast<int *>(
            simulation_speed.available_values)[curr_speed_idx];

        ConfigurationOption use_toroidal_array = *config->options[2];
        int use_toroidal_array_choice_idx =
            use_toroidal_array.currently_selected;
        const char *toroidal_array_choice = static_cast<const char **>(
            use_toroidal_array.available_values)[use_toroidal_array_choice_idx];
        game_config->use_toroidal_array =
            extract_yes_or_no_option(toroidal_array_choice);
}

StateEvolution take_simulation_step(Grid grid,
                                    SquareCellGridDimensions *dimensions,
                                    bool use_toroidal_array)
{
        // This assumes that the grid is rectangular.
        int rows = dimensions->rows;
        int cols = dimensions->cols;
        int total_cells = rows * cols;

        Grid new_grid = allocate_grid(total_cells);
        for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                        GameOfLifeCell current_state =
                            get_cell(x, y, cols, grid);
                        LOG_TRACE(TAG,
                                  "Processing cell at (%d, %d) with state %d",
                                  x, y, current_state);
                        int alive_nb = 0;
                        Point curr = {.x = x, .y = y};

                        std::vector<Point> neighbours;

                        if (use_toroidal_array) {
                                neighbours = get_neighbours_toroidal_array(
                                    &curr, rows, cols);
                        } else {
                                neighbours = get_neighbours_inside_grid(
                                    &curr, rows, cols);
                        }

                        for (Point nb : neighbours) {
                                if (get_cell(nb.x, nb.y, cols, grid) == ALIVE) {
                                        alive_nb++;
                                }
                        }

                        GameOfLifeCell new_state = EMPTY;

                        if (current_state == ALIVE) {
                                // Underpopulation or overpopulation
                                if (alive_nb < 2 || alive_nb > 3) {
                                        new_state = EMPTY;
                                } else {
                                        // Lives on to the next generation as it
                                        // has exactly 2 or 3 neighbours.
                                        new_state = ALIVE;
                                }
                        } else if (alive_nb == 3) {
                                new_state = ALIVE; // Reproduction
                        }

                        set_cell(x, y, cols, new_grid, new_state);
                }
        }
        return std::make_pair(grid, new_grid);
}

void render_state_change(Display *display, StateEvolution evolution,
                         SquareCellGridDimensions *dimensions)
{
        int rows = dimensions->rows;
        int cols = dimensions->cols;

        for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                        GameOfLifeCell prev =
                            get_cell(x, y, cols, evolution.first);
                        GameOfLifeCell curr =
                            get_cell(x, y, cols, evolution.second);

                        if (curr != prev) {
                                Color color = curr == ALIVE ? White : Black;
                                Point position = {.x = x, .y = y};
                                draw_game_cell(display, &position, dimensions,
                                               color);
                        }
                }
        }
}

void save_grid_state_in_rewind_buffer(std::vector<Grid> *rewind_buffer,
                                      int *rewind_buf_idx, Grid grid)
{

        // We need to increment the index and wrap it around as we are using
        // a ring buffer.
        *rewind_buf_idx = (*rewind_buf_idx + 1) % rewind_buffer->size();
        int idx = *rewind_buf_idx;
        LOG_DEBUG(TAG, "Current rewind buffer index is now %d",
                  *rewind_buf_idx);
        LOG_DEBUG(TAG, "Adding current state to rewind buffer at index %d",
                  *rewind_buf_idx);
        if ((*rewind_buffer)[idx] != nullptr) {
                LOG_DEBUG(TAG,
                          "Rewind buffer already has saved state at index %d, "
                          "freeing it",
                          *rewind_buf_idx);
                delete (*rewind_buffer)[idx];
        }
        (*rewind_buffer)[idx] = grid;
}

Grid handle_rewind(Direction dir, std::vector<Grid> *rewind_buffer,
                   int latest_state_idx, int *rewind_buf_idx, Grid grid,
                   SquareCellGridDimensions *gd, Display *display)
{
        // Ignore irrelevant input.
        if (dir == UP || dir == DOWN) {
                return grid;
        }

        // First we short-circuit if the user tries to go back too far.
        bool forward_in_time = dir == RIGHT;
        bool back_in_time = dir == LEFT;
        // Rewind cannot go back in time past oldest state as it would
        // wrap around to the latest state.
        if (back_in_time &&
            *rewind_buf_idx == (latest_state_idx + 1) % rewind_buffer->size()) {
                return grid;
        }

        if (forward_in_time) {
                auto next_state = (*rewind_buffer)[*rewind_buf_idx];
                render_state_change(display, std::make_pair(grid, next_state),
                                    gd);

                // Rewind cannot go into the future.
                if (*rewind_buf_idx == latest_state_idx) {
                        return grid;
                }
                *rewind_buf_idx = (*rewind_buf_idx + 1) % rewind_buffer->size();
                return next_state;
        } else if (back_in_time) {
                auto previous_state = (*rewind_buffer)[*rewind_buf_idx];

                render_state_change(display,
                                    std::make_pair(grid, previous_state), gd);
                // We need to use proper modulo as % is weird with
                // negative numbers.
                *rewind_buf_idx = mathematical_modulo((*rewind_buf_idx - 1),
                                                      rewind_buffer->size());

                /* If the rewind ring buffer has not been fully populated yet,
                   trying to rewind back in time would wrap around to the end of
                   the array and try to render nullptr diffs (as the indices
                   larger than the latest_state_idx haven't been initialized
                   yet). We need to short-circuit if that happens. */
                auto next_diffs = (*rewind_buffer)[*rewind_buf_idx];
                if (next_diffs == nullptr) {
                        LOG_DEBUG(TAG,
                                  "Rewind buffer is empty at index %d, "
                                  "skipping index update",
                                  *rewind_buf_idx);
                        // We need to increment the index to go back to safety
                        *rewind_buf_idx =
                            (*rewind_buf_idx + 1) % rewind_buffer->size();
                }
                return previous_state;
        }
        return grid;
}

void spawn_cells_randomly(Display *display, Grid grid,
                          SquareCellGridDimensions *dimensions)
{
        for (int y = 0; y < dimensions->rows; y++) {
                for (int x = 0; x < dimensions->cols; x++) {
                        // We use 30% chance os spawning a cell to avoid massive
                        // overpopulation
                        if (rand() % 10 <= 3) {
                                set_cell(x, y, dimensions->cols, grid, ALIVE);
                                Point position = {.x = x, .y = y};
                                draw_game_cell(display, &position, dimensions,
                                               White);
                        }
                }
        }
}

void draw_caret(Display *display, Point *grid_position,
                SquareCellGridDimensions *dimensions, Color caret_color)
{

        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions->left_horizontal_margin +
                 grid_position->x * GAME_CELL_WIDTH + border_offset,
            .y = dimensions->top_vertical_margin +
                 grid_position->y * GAME_CELL_WIDTH + border_offset};

        display->draw_rectangle(
            actual_position, GAME_CELL_WIDTH - 2 * border_offset,
            GAME_CELL_WIDTH - 2 * border_offset, caret_color, 1, false);
}

void draw_game_cell(Display *display, Point *grid_position,
                    SquareCellGridDimensions *dimensions, Color color)
{
        Point actual_position = {.x = dimensions->left_horizontal_margin +
                                      grid_position->x * GAME_CELL_WIDTH,
                                 .y = dimensions->top_vertical_margin +
                                      grid_position->y * GAME_CELL_WIDTH};

        display->clear_region(actual_position,
                              {.x = actual_position.x + GAME_CELL_WIDTH,
                               .y = actual_position.y + GAME_CELL_WIDTH},
                              color);
}

void erase_caret(Display *display, Point *grid_position,
                 SquareCellGridDimensions *dimensions,
                 Color grid_background_color)
{

        // We need to ensure that the caret is rendered INSIDE the text
        // cell and its border doesn't overlap the neighbouring cells.
        // Otherwise, we'll get weird rendering artifacts.
        int border_offset = 1;
        Point actual_position = {
            .x = dimensions->left_horizontal_margin +
                 grid_position->x * GAME_CELL_WIDTH + border_offset,
            .y = dimensions->top_vertical_margin +
                 grid_position->y * GAME_CELL_WIDTH + border_offset};

        display->draw_rectangle(actual_position,
                                GAME_CELL_WIDTH - 2 * border_offset,
                                GAME_CELL_WIDTH - 2 * border_offset,
                                grid_background_color, 1, false);
}

void render_help_hints(Display *display, SquareCellGridDimensions *dimensions,
                       int border_offset)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;

        int text_below_grid_y = y_margin + actual_height + 1 * border_offset;
        int r = 2;
        int d = 2 * r;
        int circle_y_axis = text_below_grid_y + FONT_SIZE / 2 + r / 4;
        const char *select = "Spawn";
        int select_len = strlen(select) * FONT_WIDTH;
        const char *exit = "Exit";
        int exit_len = strlen(exit) * FONT_WIDTH;
        const char *pause = "Pause";
        int pause_len = strlen(pause) * FONT_WIDTH;
        // We calculate the even spacing for the two indicators
        int explanations_num = 3;
        int circles_width = explanations_num * d;
        int text_circle_spacing_width = d;
        int total_width = select_len + exit_len + pause_len + circles_width +
                          (explanations_num - 1) * text_circle_spacing_width;
        int available_width = display->get_width() - 2 * x_margin;
        int remainder_space = available_width - total_width;
        int even_separator = remainder_space / explanations_num;

        int green_circle_x = x_margin + even_separator;
        display->draw_circle({.x = green_circle_x, .y = circle_y_axis}, r,
                             Green, 0, true);

        int spawn_text_x = green_circle_x + d;
        display->draw_string({.x = spawn_text_x, .y = text_below_grid_y},
                             (char *)select, FontSize::Size16, Black, White);

        int yellow_circle_x = spawn_text_x + select_len + even_separator;
        display->draw_circle({.x = yellow_circle_x, .y = circle_y_axis}, r,
                             Yellow, 0, true);
        int pause_text_x = yellow_circle_x + d;
        display->draw_string({.x = pause_text_x, .y = text_below_grid_y},
                             (char *)pause, FontSize::Size16, Black, White);

        int red_circle_x = pause_text_x + select_len + even_separator;
        display->draw_circle({.x = red_circle_x, .y = circle_y_axis}, r, Red, 0,
                             true);
        int exit_text_x = red_circle_x + d;
        display->draw_string({.x = exit_text_x, .y = text_below_grid_y},
                             (char *)exit, FontSize::Size16, Black, White);

        /* Rendering of help indicators above the grid */
        int text_grid_spacing = 4;
        // Because of slightly different font dimensions, we need this offset
        // override to ensure proper vertical space above the game grid.
        int text_above_grid_y = y_margin - border_offset - FONT_SIZE -
                                EXPLANATION_ABOVE_GRID_OFFEST;
        int circle_y_axis_above_grid =
            text_above_grid_y + (FONT_SIZE / 2 + r / 2);

        const char *toggle = "Rewind mode on/off";
        int toggle_len = strlen(toggle) * FONT_WIDTH;

        int total_width_above_grid = toggle_len + d + text_circle_spacing_width;
        int centering_margin = (available_width - total_width_above_grid) / 2;

        int blue_circle_x = x_margin + centering_margin;
        display->draw_circle(
            {.x = blue_circle_x, .y = circle_y_axis_above_grid}, r, DarkBlue, 0,
            true);
        int toggle_text_x = blue_circle_x + d;
        display->draw_string({.x = toggle_text_x, .y = text_above_grid_y},
                             (char *)toggle, FontSize::Size16, Black, White);
}

void draw_rewind_mode_indicator(Platform *p,
                                SquareCellGridDimensions *dimensions,
                                UserInterfaceCustomization *customization)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;
        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;
        int border_width = 1;
        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially)
        int border_offset = 2;

        Color indicator_border_color;

        /* The indicator border is supposed to resmble the blue color of the
           button that toggles the rewind mode. However, if the accent color is
           blue, we need to use a different color for the border to make it
           visible against the background. In this case, we use cyan as the
           border color. */

        if (customization->accent_color == DarkBlue) {
                indicator_border_color = Cyan;
        } else {
                indicator_border_color = DarkBlue;
        }

        p->display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            indicator_border_color, border_width, false);
}

void clear_rewind_mode_indicator(Platform *p,
                                 SquareCellGridDimensions *dimensions,
                                 UserInterfaceCustomization *customization)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;
        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;
        int border_width = 1;
        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially)
        int border_offset = 2;

        Color indicator_border_color;

        p->display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            customization->accent_color, border_width, false);
}

inline bool get_cell(int x, int y, int cols, Grid grid)
{
        int grid_idx = y * cols + x;
        int byte_idx = grid_idx / 8;
        int inside_byte_idx = grid_idx % 8;

        // We need to and with 1 to truncate higher bits of the selected byte.
        return (grid[byte_idx] >> inside_byte_idx) & 1;
}

inline void set_cell(int x, int y, int cols, Grid grid, bool alive)
{
        int grid_idx = y * cols + x;
        int byte_idx = grid_idx / 8;
        int inside_byte_idx = grid_idx % 8;

        if (alive) {
                grid[byte_idx] |= (1 << inside_byte_idx);
        } else {
                grid[byte_idx] &= ~(1 << inside_byte_idx);
        }
}

/**
 * The grid is stored as a bitset. Note that in order to fit the total number
 * of cells, we need to ensure that we take the ceilling of the division of the
 * nubmer of cells by 8. The reason we divide by 8 is that the grid is
 * represented as an array of bytes each of which having 8 bits.
 */
inline Grid allocate_grid(int cells)
{
        int size_in_bytes_ceiling = (cells + 7) / 8;
        auto grid = new uint8_t[size_in_bytes_ceiling];

        for (int i = 0; i < size_in_bytes_ceiling; i++) {
                grid[i] = 0;
        }
        return grid;
}
