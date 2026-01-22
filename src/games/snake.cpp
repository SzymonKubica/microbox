#include "snake.hpp"
#include "2048.hpp"
#include "game_of_life.hpp"
#include "settings.hpp"
#include <cassert>
#include <optional>
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50

#define TAG "snake"

SnakeConfiguration DEFAULT_CONFIG = {.speed = 6,
                                     .allow_grace = false,
                                     .enable_poop = true,
                                     .allow_pause = false};

namespace SnakeDefs
{
enum class Cell : uint8_t {
        Empty,
        Snake,
        Apple,
        // After an apple is eaten this is left on the grid. Note that from
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Empty`
        Poop,
        // Corresponds to a segment of snake's body that has an apple
        // inside of it (this happens after the snake eats an apple). From the
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Snake` but it gets rendered differently.
        AppleSnake,
};

/**
 * Structure bundling up all flags / counters that are required to manage the
 * state of an ongoing game loop.
 */
struct GameLoopState {
        int move_period;
        int iteration;
        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'pause' button for too long and
        // the game stutters instead of being properly paused.
        bool action_input_on_last_iteration;
        bool is_game_over;
        // To make the UX more forgiving, if the user is about to bump into a
        // wall we allow for a 'grace' period. This means that instead of
        // failing the game immediately, we wait for an additonal snake movement
        // tick and let the user change the direction. This requires rolling
        // back the changes that we have already applied on the head of the
        // snake and its segment body.
        bool grace_used;
        bool is_paused;

      public:
        GameLoopState(int moves_per_second)
            : iteration(0), action_input_on_last_iteration(false),
              is_game_over(false), grace_used(false), is_paused(false)
        {
                this->move_period = (1000 / moves_per_second) / GAME_LOOP_DELAY;
        }

        void increment_iteration()
        {
                iteration += 1;
                iteration %= move_period;
        }
        void toggle_pause() { is_paused = !is_paused; }

        /*
         * Informs us whether a sufficient number of waiting iterations has
         * passed to take a game loop step (e.g. advance the snake by one unit
         * forward).
         */
        bool is_waiting() { return iteration != move_period - 1; }
};
} // namespace SnakeDefs

using namespace SnakeDefs;

struct SnakeEntity {
        Point head;
        Point tail;
        Direction direction;
        std::vector<Point> body;

      public:
        SnakeEntity(Point head, Direction direction)
            : head(head), direction(direction)
        {
                Point tail = head;
                translate(&tail, get_opposite(direction));
                this->tail = tail;
                body = {this->tail, head};
        }

        /**
         * Moves the snake one unit along its current direction.
         */
        void take_step() { translate(&this->head, this->direction); }

        Point get_neck() { return *(this->body.end() - 2).base(); }
};

UserAction snake_loop(Platform *p, UserInterfaceCustomization *customization);

void Snake::game_loop(Platform *p, UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Use the joystick to control where the snake goes."
            "Consume apples to grow the snake. Avoid hitting the walls or "
            "snake's tail. Press yellow to (un-)pause.";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (snake_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_DEBUG(TAG, "Snake game loop finished. "
                                       "Pausing for input ");
                        Direction dir;
                        Action act;
                        pause_until_input(p->directional_controllers,
                                          p->action_controllers, &dir, &act,
                                          p->delay_provider, p->display);

                        if (act == Action::BLUE) {
                                exit_requested = true;
                        }
                        break;
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_DEBUG(TAG, "User requested snake help screen");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                }
        }
}

Point spawn_apple(std::vector<std::vector<Cell>> *grid);
void refresh_grid_cell(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, Point &location);
void render_segment_connection(Display *display, Color snake_color,
                               SquareCellGridDimensions *dimensions,
                               std::vector<std::vector<Cell>> *grid,
                               Point &first_location, Point &second_location);
void render_snake_head(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, const Point &head,
                       Direction direction);
void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score);

UserAction snake_loop(Platform *p, UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering Snake game loop");
        SnakeConfiguration config;

        auto maybe_interrupt = collect_snake_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        int game_cell_width = 10;
        SquareCellGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), game_cell_width);
        int rows = gd->rows;
        int cols = gd->cols;

        LOG_DEBUG(TAG, "Rendering snake game area.");
        draw_grid_frame(p, customization, gd);

        std::vector<std::vector<Cell>> grid(rows, std::vector<Cell>(cols));

        // We render the 'Score:' text only once but including the empty space
        // required for the score. This is needed to ensure that the score
        // section above the grid is properly centered. When rendering the
        // actual score count, we need to cancel out the three spaces and
        // subtract them from score_end pixel position. The function above
        // returns the ending position of the score text so that we can render
        // the score appropriately.
        int score_text_end_x = render_centered_above_frame(p, gd, "Score:    ");

        /*
         * Helper lambda expressions to avoid passing platform / context
         * parametrs on each call site. The idea is to capture all parameters
         * that don't change during the game.
         */

        // Sets the required location on the grid to a provided value.
        auto set_cell = [&grid](const Point &location, Cell value) {
                grid[location.y][location.x] = value;
        };
        // Performs a lookup of the grid value without explicit array indexing.
        auto get_cell = [&grid](const Point &location) {
                return grid[location.y][location.x];
        };
        // Re-renders the score in the correct location above the grid as
        // determined by grid dimensions and the score text end x position.
        auto render_score = [p, gd, &grid, score_text_end_x](int score) {
                update_score(p, gd, score_text_end_x, score);
        };
        // After the value of a given cell in the grid is changed, this
        // re-renders that single cell in the display.
        auto render_cell = [p, gd, &grid, customization](Point &location) {
                refresh_grid_cell(p->display, customization->accent_color, gd,
                                  &grid, location);
        };
        // Renders the snake's head including the neck (2nd segment right behind
        // the head).
        auto render_head = [p, gd, &grid, customization](SnakeEntity &snake) {
                auto neck = snake.get_neck();
                render_segment_connection(p->display,
                                          customization->accent_color, gd,
                                          &grid, neck, snake.head);
                render_snake_head(p->display, customization->accent_color, gd,
                                  &grid, snake.head, snake.direction);
        };

        // Initial score rendering to complete the game grid first, before the
        // snake gets rendered.
        render_score(0);
        p->display->refresh();

        // Initialize game entities
        // The snake starts in the middle of the area pointing to the right.
        SnakeEntity snake{{.x = cols / 2, .y = rows / 2}, Direction::RIGHT};
        set_cell(snake.head, Cell::Snake);
        set_cell(snake.tail, Cell::Snake);
        Point apple_location = spawn_apple(&grid);

        // Render initial state of the game entities.
        render_cell(snake.tail);
        render_head(snake);
        render_cell(apple_location);

        GameLoopState state{config.speed};

        // Convenience funtion to ensure each short-circuit exit of the
        // loop iteration actually increments the counter and waits a bit.
        auto increment_iteration_and_wait = [p, &state]() {
                state.increment_iteration();
                p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                p->display->refresh();
        };

        // We let the user change the new snake direction at any point during
        // the frame but it gets applied on the snake only at the end of the
        // given frame. The reason for this is that doing it each time an input
        // is registered led to issues where quick changes to the direction
        // could make the snake go opposite (turn on the spot) and fail the
        // game.
        Direction chosen_snake_direction = snake.direction;
        int game_score = 0;
        while (!state.is_game_over) {
                Direction dir;
                Action act;
                if (poll_directional_input(p->directional_controllers, &dir) &&
                    !is_opposite(dir, snake.direction)) {
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        chosen_snake_direction = dir;
                }

                bool action_taken =
                    poll_action_input(p->action_controllers, &act);

                if (config.allow_pause && action_taken && act == YELLOW &&
                    !state.action_input_on_last_iteration) {
                        state.toggle_pause();
                        state.action_input_on_last_iteration = true;
                } else if (!action_taken) {
                        // We clear the action-held flag only if there is no
                        // action registered. This is done to ensure that
                        // if a pause button is held for a long time, the game
                        // remains paused until it is depressed and clicked
                        // again.
                        state.action_input_on_last_iteration = false;
                }

                // If we are paused or it is not the time to move yet, we finish
                // processing early.
                if (state.is_paused || state.is_waiting()) {
                        increment_iteration_and_wait();
                        continue;
                }

                Cell next;
                snake.direction = chosen_snake_direction;
                snake.take_step();

                // Check for failure conditions.
                bool wall_hit = is_out_of_bounds(&(snake.head), gd);
                if (!wall_hit) {
                        next = get_cell(snake.head);
                }
                bool tail_hit = next == Cell::Snake || next == Cell::AppleSnake;

                if (wall_hit || tail_hit) {
                        if (!config.allow_grace || state.grace_used) {
                                LOG_INFO(TAG, "Snake game is over.");
                                state.is_game_over = true;
                                break;
                        }

                        // We allow the user to change the direction for an
                        // additional tick by rolling back the head position.
                        Point previous_head = *(snake.body.end() - 1);
                        snake.head = previous_head;
                        state.grace_used = true;
                        increment_iteration_and_wait();
                        continue;
                }

                // If we got here, it means that the next cell is within bounds
                // and is not occupied by the snake body. This means that we can
                // safely clear grace.
                state.grace_used = false;

                // The snake has entered the next location, if the next location
                // is an apple, we mark it as 'segment of snake with an apple in
                // its stomach' and render differently
                auto next_segment =
                    next == Cell::Apple ? Cell::AppleSnake : Cell::Snake;
                set_cell(snake.head, next_segment);

                // We need to draw the small rectangle that connects the new
                // snake head to the rest of its body. This is needed as Snake's
                // head needs to have a different shape from all other segments.
                // Because of this, we need to update the neck here to actually
                // render it's proper contents (i.e. whether it is a regular
                // snake segment or a segment that contains an apple).
                snake.body.push_back(snake.head);
                auto neck = snake.get_neck();
                render_cell(neck);
                render_head(snake);

                if (next == Cell::Apple) {
                        // Eating an apple is handled by simply skipping the
                        // step where we erase the last segment of the snake
                        // We then spawn a new apple.
                        Point apple_loc = spawn_apple(&grid);
                        game_score++;
                        render_cell(apple_loc);
                        render_score(game_score);
                        increment_iteration_and_wait();
                        continue;
                }

                // If we got here it must be safe to advance the snake and
                // erase the last tail location as no apple is consumed.
                assert(next == Cell::Empty || next == Cell::Poop);
                auto tail_iter = snake.body.begin();
                auto tail = *tail_iter;

                Cell updated = Cell::Empty;
                // If the last segment contains an apple and poop
                // visuals are enabled, we leave poop behind the snake.
                if (config.enable_poop && get_cell(tail) == Cell::AppleSnake) {
                        updated = Cell::Poop;
                }

                set_cell(tail, updated);
                render_cell(tail);
                snake.body.erase(tail_iter);
                increment_iteration_and_wait();
        }

        p->display->refresh();
        return UserAction::PlayAgain;
}

/**
 * Re-renders the text location above the grid informing the user about the
 * current score in the game.
 */
void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score)
{

        char buffer[4];
        sprintf(buffer, "%3d", score);
        // We start 3 letters after the end of the text end location.
        // The reason for this is that the 'Score:' text is rendered centered
        // using the render_centered_text_above_frame function and so it
        // the string is right-padded with exactly 4 spaces to make enough
        // horizontal space for the score digits to be printed (and have the
        // entire text above the grid centered). Hence we need to start
        // rendering the actual digits starting after the first space after the
        // colon in 'Score:' text, so we move the start location by 3 spaces.
        int start_position = score_text_end_location - 3 * FONT_WIDTH;
        render_text_above_frame_starting_from(p, dimensions, buffer,
                                              start_position, true);
}

Point spawn_apple(std::vector<std::vector<Cell>> *grid)
{
        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        while (true) {
                int x = rand() % cols;
                int y = rand() % rows;

                Cell selected = (*grid)[y][x];
                if (selected != Cell::Snake) {
                        (*grid)[y][x] = Cell::Apple;
                        return {x, y};
                        break;
                }
        }
}

/**
 * Renders a segment that connects two adjacent locations on the grid.
 * Note that this function assumes that the two points are adjacent and the
 * caller of the function needs to ensure that this is indeed the case.
 * Also note that here we are referring to the 'logical' locations, i.e. cells
 * on the grid, not the actual pixel locations. This function performs the
 * translation between the two concepts.
 */
void render_segment_connection(Display *display, Color snake_color,
                               SquareCellGridDimensions *dimensions,
                               std::vector<std::vector<Cell>> *grid,
                               Point &first_location, Point &second_location)
{

        // Calculation logic to map from logical cells to the actual pixel
        // values. Note that this is a duplicate of the same logic in
        // `re_render_grid_cell` and could be refactored into some other
        // function or merged into grid dimensions struct if it becomes useful
        // somewhere else.
        int padding = 2;
        // Rectangle border width needs to be non-zero, else the physical LCD
        // display will not render rectangles.
        int border_width = 1;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        bool adjacent_horizontally = first_location.y == second_location.y;

        LOG_DEBUG(TAG,
                  "Rendering segment connection between: {x: %d, y: %d} and "
                  "{x: %d, y: %d}",
                  first_location.x, first_location.y, second_location.x,
                  second_location.y);

        Point start;
        int segment_width;
        int segment_height;
        if (adjacent_horizontally) {
                Point &left_point = first_location.x < second_location.x
                                        ? first_location
                                        : second_location;

                // We start drawing from the end of the padded square, hence we
                // add `width - padding below to get to that point` also note
                // that we need to start drawing from the padded vertical start
                // hence we add the padding in the y coordinate
                start = {.x = left_margin + (left_point.x + 1) * width,
                         .y = top_margin + left_point.y * height + padding};
                segment_width = padding - border_width;
                segment_height = height - 2 * padding;

        } else {
                Point &top_point = first_location.y < second_location.y
                                       ? first_location
                                       : second_location;
                // We start drawing from the end of the padded square (bottom
                // left corner), hence we add `height - padding below to get to
                // that point`
                start = {.x = left_margin + top_point.x * width + padding,
                         .y = top_margin + (top_point.y + 1) * height};
                segment_width = width - 2 * padding;
                segment_height = padding - border_width;
        }

        display->draw_rectangle(start, segment_width, segment_height,
                                snake_color, border_width, true);
}

/**
 * Renders the head of the snake. This contains snake's eye and a rounded front
 * part of the snake.
 */
void render_snake_head(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, const Point &head,
                       Direction direction)
{

        // Calculation logic to map from logical cells to the actual pixel
        // values. Note that this is a duplicate of the same logic in
        // `re_render_grid_cell` and could be refactored into some other
        // function or merged into grid dimensions struct if it becomes useful
        // somewhere else.
        int padding = 2;
        // rectangle border width needs to be non-zero, else the physical LCD
        // display will not render rectangles.
        int border_width = 1;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        // We start drawing from the end of the padded square, hence we
        // add `width - padding` below to get to that point. Also note
        // that we need to start drawing from the padded vertical start
        // hence we add the padding in the y coordinate
        Point start = {.x = left_margin + head.x * width,
                       .y = top_margin + head.y * height};

        // We draw a 'half-cell' to connect snake head to the neck.
        int snake_w = width - 2 * padding;
        int snake_h = height - 2 * padding;
        int rectangle_w, rectangle_h;

        switch (direction) {
        case UP:
        case DOWN:
                rectangle_w = snake_w;
                rectangle_h = snake_h / 2;
                break;
        case RIGHT:
        case LEFT:
                rectangle_w = snake_w / 2;
                rectangle_h = snake_h;
        }

        Point offset, eye_offset;
        // When going down we need to make the rectangle a bit larger.
        // TODO: clean up
        int vertical_extension = 0;
        int height_adj = 0;
        switch (direction) {
        case UP:
                offset = {.x = 0, .y = snake_h / 2};
                eye_offset = {.x = -rectangle_w / 4, .y = 0};
                height_adj = 1;
                break;
        case LEFT:
                offset = {.x = snake_w / 2, .y = 0};
                eye_offset = {.x = 0, .y = rectangle_h / 4};
                break;
        case DOWN:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = rectangle_w / 4, .y = 0};
                vertical_extension = 3;
                break;
        case RIGHT:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = 0, .y = -rectangle_h / 4};
                break;
        }

        display->draw_rectangle(
            {.x = start.x + offset.x + padding,
             .y = start.y + offset.y + padding - vertical_extension},
            rectangle_w, rectangle_h + vertical_extension + height_adj,
            snake_color, border_width, true);
        Point cell_center = {.x = start.x + width / 2,
                             .y = start.y + height / 2};
        Point eye_center = {
            .x = start.x + offset.x + padding + rectangle_w / 2 + eye_offset.x,
            .y = start.y + offset.y + padding + rectangle_h / 2 + eye_offset.y};
        display->draw_circle(cell_center, (width - 2 * padding) / 2,
                             snake_color, border_width, true);
        display->draw_circle(eye_center, 1, Black, 0, true);
}

void refresh_grid_cell(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, Point &location)
{
        int padding = 2;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        Point start = {.x = left_margin + location.x * width,
                       .y = top_margin + location.y * height};

        Cell cell_type = (*grid)[location.y][location.x];

        // When rendering on the actual lcd display the circle comes out a bit
        // larger because of pixel inaccuracies and internal of that lcd display
        // driver. Because of this we need to decrease the radius slightly.
        int radius_offset = 1;

        // Represents the top left corner of the snake segment
        Point padded_start = {.x = start.x + padding, .y = start.y + padding};
        Point apple_center = {.x = start.x + width / 2,
                              .y = start.y + width / 2};

        switch (cell_type) {
        case Cell::Apple: {
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Red, 0, true);
                break;
        }
        case Cell::Snake: {
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                break;
        }
        case Cell::Empty: {
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                break;
        }
        case Cell::AppleSnake: {
                // Here we render a normal snake segment that has a hole inside
                // of it with an apple sitting there. This is to indicate
                // segments of the snake that have 'consumed an apple'
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Black, 0, true);
                display->draw_circle(
                    apple_center, (width - 2 * padding) / 2 - 2 * radius_offset,
                    Color::Red, 0, true);
                break;
        }
        case Cell::Poop: {

                // First we clear the cell and then we draw a pile of shit.
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                auto locations = {
                    apple_center,
                    {.x = apple_center.x + 2, .y = apple_center.y},
                    {.x = apple_center.x + 1, .y = apple_center.y - 2}};
                for (Point loc : locations) {
                        display->draw_circle(
                            loc, (width - 2 * padding) / 2 - radius_offset,
                            Color::Brown, 0, true);
                }
                break;
        }
        }
}

Configuration *assemble_snake_configuration(PersistentStorage *storage);
void extract_game_config(SnakeConfiguration *game_config,
                         Configuration *config);
std::optional<UserAction>
collect_snake_config(Platform *p, SnakeConfiguration *game_config,
                     UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_snake_configuration(p->persistent_storage);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        free_configuration(config);
        return std::nullopt;
}

SnakeConfiguration *load_initial_snake_config(PersistentStorage *storage);
Configuration *assemble_snake_configuration(PersistentStorage *storage)
{
        SnakeConfiguration *initial_config = load_initial_snake_config(storage);

        ConfigurationOption *speed = ConfigurationOption::of_integers(
            "Speed", {4, 5, 6, 7, 8}, initial_config->speed);

        auto *poop = ConfigurationOption::of_strings(
            "Poop", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->enable_poop));

        auto *allow_grace = ConfigurationOption::of_strings(
            "Grace period", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_grace));

        auto *allow_pause = ConfigurationOption::of_strings(
            "Allow pause", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_pause));

        std::vector<ConfigurationOption *> options = {speed, poop, allow_grace,
                                                      allow_pause};

        return new Configuration("Snake", options);
}

SnakeConfiguration *load_initial_snake_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offsets()[Snake];
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        // We initialize empty config to detect corrupted memory and fallback
        // to defaults if needed.
        SnakeConfiguration config = {.speed = 0,
                                     .allow_grace = 0,
                                     .enable_poop = false,
                                     .allow_pause = 0};

        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage->get(storage_offset, config);

        SnakeConfiguration *output = new SnakeConfiguration();

        if (config.speed == 0) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "snake configuration, using default values.");
                memcpy(output, &DEFAULT_CONFIG, sizeof(SnakeConfiguration));
                storage->put(storage_offset, DEFAULT_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(SnakeConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded snake configuration: speed=%d, enable_poop=%d, "
                  "allow_grace=%d"
                  "allow_pause=%d",
                  output->speed, output->enable_poop, output->allow_grace,
                  output->allow_pause);

        return output;
}

void extract_game_config(SnakeConfiguration *game_config, Configuration *config)
{
        ConfigurationOption speed = *config->options[0];
        ConfigurationOption enable_poop = *config->options[1];
        ConfigurationOption allow_grace = *config->options[2];
        ConfigurationOption allow_pause = *config->options[3];

        auto yes_or_no_option_to_bool = [](ConfigurationOption option) {
                return extract_yes_or_no_option(option.get_current_str_value());
        };

        game_config->speed = speed.get_curr_int_value();
        game_config->enable_poop = yes_or_no_option_to_bool(enable_poop);
        game_config->allow_grace = yes_or_no_option_to_bool(allow_grace);
        game_config->allow_pause = yes_or_no_option_to_bool(allow_pause);
}
