#include "snake.hpp"
#include "2048.hpp"
#include "game_of_life.hpp"
#include "settings.hpp"
#include <cassert>
#include <optional>
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50

#define TAG "snake"

SnakeConfiguration DEFAULT_SNAKE_CONFIG = {
    .speed = 2, .allow_grace = false, .accelerate = true, .allow_pause = false};

enum class SnakeGridCell : uint8_t {
        Empty,
        Snake,
        // Equivalent to `SnakeGridCell::Snake` but with different rendering.
        SnakeHead,
        Apple,
        // After an apple is eaten this is left on the grid. Note that from
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Empty`
        Poop,
        // Corresponds to a segment of snake's body that has an apple
        // inside of it (this happens after the snake eats an apple). From the
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Snake` but it gets rendered differently.
        SnakeWithApple,
};

struct SnakeEntity {
        Point head;
        Point tail;
        Direction direction;
        std::vector<Point> *body;
};

UserAction snake_loop(Platform *p, UserInterfaceCustomization *customization);

void Snake::game_loop(Platform *p, UserInterfaceCustomization *customization)
{
        const char *help_text = "Some snake help";

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
            "Speed", {1, 2, 3, 4, 5, 6, 7, 8}, initial_config->speed);

        auto *accelerate = ConfigurationOption::of_strings(
            "Accelerate", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->accelerate));

        auto *allow_grace = ConfigurationOption::of_strings(
            "Grace period", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_grace));

        auto *allow_pause = ConfigurationOption::of_strings(
            "Allow pause", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_pause));

        std::vector<ConfigurationOption *> options = {speed, accelerate,
                                                      allow_grace, allow_pause};

        return new Configuration("Snake", options, "Start Game");
}

SnakeConfiguration *load_initial_snake_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offsets()[Snake];
        LOG_DEBUG(TAG, "Loading snake saved config from offset %d",
                  storage_offset);

        // We initialize empty config to detect corrupted memory and fallback
        // to defaults if needed.
        SnakeConfiguration config = {.speed = 0,
                                     .allow_grace = 0,
                                     .accelerate = false,
                                     .allow_pause = 0};

        LOG_DEBUG(
            TAG, "Trying to load initial settings from the persistent storage");
        storage->get(storage_offset, config);

        SnakeConfiguration *output = new SnakeConfiguration();

        if (config.speed == 0) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "snake configuration, using default values.");
                memcpy(output, &DEFAULT_SNAKE_CONFIG,
                       sizeof(SnakeConfiguration));
                storage->put(storage_offset, DEFAULT_SNAKE_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(SnakeConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded snake  game configuration: speed=%d, accelerate=%d, "
                  "allow_pause=%d",
                  output->speed, output->accelerate, output->allow_pause);

        return output;
}

void extract_game_config(SnakeConfiguration *game_config, Configuration *config)
{
        ConfigurationOption speed = *config->options[0];
        ConfigurationOption accelerate = *config->options[1];
        ConfigurationOption allow_grace = *config->options[2];
        ConfigurationOption allow_pause = *config->options[3];

        auto yes_or_no_option_to_bool = [](ConfigurationOption option) {
                return extract_yes_or_no_option(option.get_current_str_value());
        };

        game_config->speed = speed.get_curr_int_value();
        game_config->accelerate = yes_or_no_option_to_bool(accelerate);
        game_config->allow_grace = yes_or_no_option_to_bool(allow_grace);
        game_config->allow_pause = yes_or_no_option_to_bool(allow_pause);
}

Point *spawn_apple(std::vector<std::vector<SnakeGridCell>> *grid);
void re_render_grid_cell(Display *display, Color snake_color,
                         GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location);
void render_segment_connection(Display *display, Color snake_color,
                               GameOfLifeGridDimensions *dimensions,
                               std::vector<std::vector<SnakeGridCell>> *grid,
                               Point *first_location, Point *second_location);
bool is_out_of_bounds(Point *p, GameOfLifeGridDimensions *dimensions);

UserAction snake_loop(Platform *p, UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering Snake game loop");
        SnakeConfiguration config;

        auto maybe_interrupt = collect_snake_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        GameOfLifeGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius());
        int rows = gd->rows;
        int cols = gd->cols;

        draw_game_canvas(p, gd, customization);
        LOG_DEBUG(TAG, "Snake game canvas drawn.");

        p->display->refresh();

        std::vector<std::vector<SnakeGridCell>> grid(
            rows, std::vector<SnakeGridCell>(cols));

        Point snake_head = {.x = cols / 2, .y = rows / 2};
        Point snake_tail = {.x = snake_head.x - 1, .y = snake_head.y};

        std::vector<Point> body;
        SnakeEntity snake = {snake_head, snake_tail, Direction::RIGHT, &body};
        snake.body->push_back(snake_tail);
        snake.body->push_back(snake_head);

        grid[snake_head.y][snake_head.x] = SnakeGridCell::SnakeHead;
        grid[snake_tail.y][snake_tail.x] = SnakeGridCell::Snake;

        // Those helper lambdas avoid passing the grid and display parameters
        // around each time we want to re-render a cell or draw a snake segment.
        auto update_display_cell = [p, gd, &grid,
                                    customization](Point *location) {
                re_render_grid_cell(p->display, customization->accent_color, gd,
                                    &grid, location);
        };
        auto draw_segment_connection = [p, gd, &grid,
                                        customization](Point *first_segment,
                                                       Point *second_segment) {
                render_segment_connection(p->display,
                                          customization->accent_color, gd,
                                          &grid, first_segment, second_segment);
        };

        update_display_cell(&snake_tail);
        draw_segment_connection(&snake_tail, &snake_head);
        update_display_cell(&snake_head);

        Point *apple_location = spawn_apple(&grid);
        update_display_cell(apple_location);

        int move_period = (1000 / config.speed) / GAME_LOOP_DELAY;
        int iteration = 0;

        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'spawn' button for too long and
        // the cell flickers instead of getting spawned properly. We implement
        // this using this flag.
        bool action_input_on_last_iteration = false;
        bool is_game_over = false;
        // To make the UX more forgiving, if the user is about to bump into a
        // wall we allow for a 'grace' period. This means that instead of
        // failing the game immediately, we wait for an additonal snake movement
        // tick and let the user change the direction. This requires rolling
        // back the changes that we have already applied on the head of the
        // snake and its segment body.
        bool grace_used = false;

        bool is_paused = false;
        while (!is_game_over) {
                Direction dir;
                Action act;
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        if (!is_opposite(dir, snake.direction)) {
                                snake.direction = dir;
                        }
                }
                if (action_input_registered(p->action_controllers, &act) &&
                    !action_input_on_last_iteration) {
                        if (act == YELLOW) {
                                is_paused = !is_paused;
                                action_input_on_last_iteration = true;
                        }
                } else {
                        action_input_on_last_iteration = false;
                }

                if (!is_paused && iteration == move_period - 1) {

                        // This modifies the snake.head in place.
                        translate(&snake.head, snake.direction);

                        bool hit_a_wall = is_out_of_bounds(&(snake.head), gd);
                        SnakeGridCell next;

                        if (!hit_a_wall) {
                                next = grid[snake.head.y][snake.head.x];
                        }
                        bool tail_bitten =
                            next == SnakeGridCell::Snake ||
                            next == SnakeGridCell::SnakeHead ||
                            next == SnakeGridCell::SnakeWithApple;
                        bool failure = hit_a_wall || tail_bitten;

                        if (!failure) {
                                // If we got here, it means that the next cell
                                // is within bounds and is not occupied by the
                                // snake body. This means that we can safely
                                // clear grace.
                                grace_used = false;

                                // The snake has entered the next location, if
                                // the next location is an apple, we mark it
                                // as 'segment of snake with an apple in its
                                // stomach' and render differently
                                grid[snake.head.y][snake.head.x] =
                                    next == SnakeGridCell::Apple
                                        ? SnakeGridCell::SnakeWithApple
                                        : SnakeGridCell::SnakeHead;

                                // We need to draw the small rectangle that
                                // connects the new snake head to the rest of
                                // its body.
                                Point *neck = (snake.body->end() - 1).base();
                                update_display_cell(&snake.head);
                                draw_segment_connection(neck, &snake.head);
                                // The neck is no longer a snake head so we
                                // update it and re-render. We only do this if the
                                // previous neck is not a snake-with-apple segment
                                auto prev_neck = grid[neck->y][neck->x];
                                if (prev_neck != SnakeGridCell::SnakeWithApple) {
                                grid[neck->y][neck->x] = SnakeGridCell::Snake;
                                update_display_cell(neck);
                                }
                                snake.body->push_back(snake.head);
                                if (next == SnakeGridCell::Apple) {
                                        // Eating an apple is handled by simply
                                        // skipping the step where we erase the
                                        // last segment of the snake (the else
                                        // branch). We then spawn a new apple.
                                        Point *apple_location =
                                            spawn_apple(&grid);
                                        re_render_grid_cell(
                                            p->display,
                                            customization->accent_color, gd,
                                            &grid, apple_location);
                                } else {
                                        assert(next == SnakeGridCell::Empty ||
                                               next == SnakeGridCell::Poop);

                                        // When no apple is consumed we move
                                        // the snake forward by erasing its last
                                        // segment.
                                        auto tail = snake.body->begin();
                                        auto previous = grid[tail->y][tail->x];
                                        auto updated =
                                            previous == SnakeGridCell::
                                                            SnakeWithApple
                                                ? SnakeGridCell::Poop
                                                : SnakeGridCell::Empty;
                                        grid[tail->y][tail->x] = updated;
                                        update_display_cell(tail.base());
                                        snake.body->erase(tail);
                                }

                        } else {
                                if (grace_used || !config.allow_grace) {
                                        is_game_over = true;
                                        break;
                                }

                                // We allow the user to change the
                                // direction for an additional tick by
                                // rolling back the head position.
                                Point previous_head = *(snake.body->end() - 1);
                                snake.head = {previous_head.x, previous_head.y};

                                grace_used = true;
                        }
                }

                iteration += 1;
                iteration %= move_period;
                p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                p->display->refresh();
        }

        p->display->refresh();
        return UserAction::PlayAgain;
}

bool is_out_of_bounds(Point *p, GameOfLifeGridDimensions *dimensions)
{
        int x = p->x;
        int y = p->y;

        return x < 0 || y < 0 || x >= dimensions->cols || y >= dimensions->rows;
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
                               GameOfLifeGridDimensions *dimensions,
                               std::vector<std::vector<SnakeGridCell>> *grid,
                               Point *first_location, Point *second_location)
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

        bool adjacent_horizontally = first_location->y == second_location->y;

        LOG_DEBUG(TAG,
                  "Rendering segment connection between: {x: %d, y: %d} and "
                  "{x: %d, y: %d}",
                  first_location->x, first_location->y, second_location->x,
                  second_location->y);

        Point start;
        if (adjacent_horizontally) {
                Point left_point = first_location->x < second_location->x
                                       ? *first_location
                                       : *second_location;

                // We start drawing from the end of the padded square, hence we
                // add `width - padding below to get to that point` also note
                // that we need to start drawing from the padded vertical start
                // hence we add the padding in the y coordinate
                start = {.x = left_margin + (left_point.x + 1) * width,
                         .y = top_margin + left_point.y * height + padding};
                display->draw_rectangle(start, padding - border_width,
                                        height - 2 * padding, snake_color,
                                        border_width, true);

        } else {
                Point top_point = first_location->y < second_location->y
                                      ? *first_location
                                      : *second_location;
                // We start drawing from the end of the padded square (bottom
                // left corner), hence we add `height - padding below to get to
                // that point`
                start = {.x = left_margin + top_point.x * width + padding,
                         .y = top_margin + (top_point.y + 1) * height};
                display->draw_rectangle(start, width - 2 * padding,
                                        padding - border_width, snake_color,
                                        border_width, true);
        }
}

void re_render_grid_cell(Display *display, Color snake_color,
                         GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location)
{
        int padding = 2;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        Point grid_loc = *location;
        Point start = {.x = left_margin + grid_loc.x * width,
                       .y = top_margin + grid_loc.y * height};

        SnakeGridCell cell_type = (*grid)[grid_loc.y][grid_loc.x];

        // When rendering on the actual lcd display the circle comes out a bit
        // larger because of pixel inaccuracies and internal of that lcd display
        // driver. Because of this we need to decrease the radius slightly.
        int radius_offset = 1;

        // Represents the top left corner of the snake segment
        Point padded_start = {.x = start.x + padding, .y = start.y + padding};
        Point apple_center = {.x = start.x + width / 2,
                              .y = start.y + width / 2};

        switch (cell_type) {
        case SnakeGridCell::Apple: {
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Red, 0, true);
                break;
        }
        case SnakeGridCell::Snake: {
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                break;
        }
        case SnakeGridCell::Empty: {
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                break;
        }
        case SnakeGridCell::SnakeWithApple: {
                // Here we render a normal snake segment that has a hole inside
                // of it with an apple sitting there. This is to indicate
                // segments of the snake that have 'consumed an apple'
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                display->draw_circle(apple_center, (width - 2 * padding) / 2,
                                     Color::Black, 0, true);
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Red, 0, true);
                break;
        }
        case SnakeGridCell::Poop: {

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
        case SnakeGridCell::SnakeHead: {
                display->draw_circle(apple_center, (width - padding) / 2,
                                     snake_color, 0, true);
                break;
        }
        }
}

Point *spawn_apple(std::vector<std::vector<SnakeGridCell>> *grid)
{
        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        while (true) {
                int x = rand() % cols;
                int y = rand() % rows;

                SnakeGridCell selected = (*grid)[y][x];
                if (selected != SnakeGridCell::Snake) {
                        (*grid)[y][x] = SnakeGridCell::Apple;
                        Point *random_position = new Point();
                        random_position->x = x;
                        random_position->y = y;
                        return random_position;
                        break;
                }
        }
}
