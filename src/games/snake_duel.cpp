#include "snake_duel.hpp"
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

SnakeDuelConfiguration DEFAULT_SNAKE_DUEL_CONFIG = {.speed = 6,
                                                    .allow_grace = false,
                                                    .enable_poop = true,
                                                    .secondary_player_color =
                                                        Blue};

enum class SnakeDuelGridCell : uint8_t {
        Empty,
        Snake,
        SecondarySnake,
        Apple,
        // After an apple is eaten this is left on the grid. Note that from
        // game mechanics perspective this is equivalent to
        // `SnakeDuelGridCell::Empty`
        Poop,
        // Corresponds to a segment of snake's body that has an apple
        // inside of it (this happens after the snake eats an apple). From the
        // game mechanics perspective this is equivalent to
        // `SnakeDuelGridCell::Snake` but it gets rendered differently.
        SnakeWithApple,
        SecondarySnakeWithApple,
};

struct SnakeEntity {
        Point head;
        Point tail;
        Direction direction;
        std::vector<Point> *body;
};

UserAction snake_duel_loop(Platform *p,
                           UserInterfaceCustomization *customization);

void SnakeDuel::game_loop(Platform *p,
                          UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Use the joystick to control where the snake goes."
            "Consume apples to grow the snake. Avoid hitting the walls or "
            "snake's tail. Second player: use keypad to control the snake.";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (snake_duel_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_DEBUG(TAG, "Snake duel game loop finished. "
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

Point *spawn_apple(std::vector<std::vector<SnakeDuelGridCell>> *grid);
void refresh_grid_cell(Display *display, Color snake_color,
                       Color secondary_snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<SnakeDuelGridCell>> *grid,
                       Point *location);
void render_segment_connection(
    Display *display, Color snake_color, SquareCellGridDimensions *dimensions,
    std::vector<std::vector<SnakeDuelGridCell>> *grid, Point *first_location,
    Point *second_location);
void render_head(Display *display, Color snake_color,
                 SquareCellGridDimensions *dimensions,
                 std::vector<std::vector<SnakeDuelGridCell>> *grid, Point *head,
                 Direction direction);

void update_duel_score(Platform *p, SquareCellGridDimensions *dimensions,
                       int score_text_end_location, int score, bool is_secondary = false);

void take_snake_step(SnakeEntity &snake, Direction chosen_snake_direction,
                     bool *grace_used, bool *is_game_over, int *game_score,
                     SnakeDuelConfiguration &config,
                     SquareCellGridDimensions *gd,
                     std::vector<std::vector<SnakeDuelGridCell>> &grid,
                     UserInterfaceCustomization *customization, Platform *p,
                     int score_end, bool is_secondary);

UserAction snake_duel_loop(Platform *p,
                           UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering Snake game loop");
        SnakeDuelConfiguration config;

        auto maybe_interrupt =
            collect_snake_duel_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        int game_cell_width = 10;
        SquareCellGridDimensions *gd = calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), game_cell_width);
        int rows = gd->rows;
        int cols = gd->cols;

        draw_grid_frame(p, customization, gd);

        // We render the 'Score:' text only once but including the empty space
        // required for the score. This is needed to ensure that the score
        // section above the grid is properly centered. When rendering the
        // actual score count, we need to cancel out the three spaces and
        // subtract them from score_end pixel position.
        int score_end =
            render_centered_text_above_frame(p, gd, (char *)"P1:    P2:    ");
        update_duel_score(p, gd, score_end, 0);
        update_duel_score(p, gd, score_end, 0, true);
        LOG_DEBUG(TAG, "Snake game area border drawn.");

        p->display->refresh();

        std::vector<std::vector<SnakeDuelGridCell>> grid(
            rows, std::vector<SnakeDuelGridCell>(cols));

        // The first snake starts in the middle of the game area pointing to the
        // right.
        Point snake_head = {.x = cols / 2, .y = rows / 2};
        Point snake_tail = {.x = snake_head.x - 1, .y = snake_head.y};
        std::vector<Point> body;
        SnakeEntity snake = {snake_head, snake_tail, Direction::RIGHT, &body};
        snake.body->push_back(snake_tail);
        snake.body->push_back(snake_head);
        grid[snake_head.y][snake_head.x] = SnakeDuelGridCell::Snake;
        grid[snake_tail.y][snake_tail.x] = SnakeDuelGridCell::Snake;

        // The first snake starts one cell below the first one going in the
        // opposite direction
        Point secondary_snake_head = {.x = cols / 2, .y = rows / 2 + 1};
        Point secondary_snake_tail = {.x = secondary_snake_head.x + 1,
                                      .y = secondary_snake_head.y};
        std::vector<Point> secondary_body;
        SnakeEntity secondary_snake = {secondary_snake_head,
                                       secondary_snake_tail, Direction::LEFT,
                                       &secondary_body};
        secondary_snake.body->push_back(secondary_snake_tail);
        secondary_snake.body->push_back(secondary_snake_head);
        grid[secondary_snake_head.y][secondary_snake_head.x] =
            SnakeDuelGridCell::SecondarySnake;
        grid[secondary_snake_tail.y][secondary_snake_tail.x] =
            SnakeDuelGridCell::SecondarySnake;

        // Those helper lambdas avoid passing the grid and display parameters
        // around each time we want to re-render a cell or draw a snake segment.
        auto update_display_cell = [p, gd, &grid, customization,
                                    &config](Point *location) {
                refresh_grid_cell(p->display, customization->accent_color,
                                  config.secondary_player_color, gd, &grid,
                                  location);
        };
        auto draw_neck_and_head =
            [p, gd, &grid](Point *first_segment, Point *second_segment,
                           SnakeEntity &snake, Color color) {
                    render_segment_connection(p->display, color, gd, &grid,
                                              first_segment, second_segment);
                    render_head(p->display, color, gd, &grid, second_segment,
                                snake.direction);
            };

        // First render of the first snake
        update_display_cell(&snake_tail);
        draw_neck_and_head(&snake_tail, &snake_head, snake,
                           customization->accent_color);
        update_display_cell(&snake_head);

        // First render of the second snake
        update_display_cell(&secondary_snake_tail);
        draw_neck_and_head(&secondary_snake_tail, &secondary_snake_head,
                           secondary_snake, config.secondary_player_color);
        update_display_cell(&secondary_snake_head);

        Point *apple_location = spawn_apple(&grid);
        update_display_cell(apple_location);

        int move_period = (1000 / config.speed) / GAME_LOOP_DELAY;
        int iteration = 0;

        // [todo]: idea - all of those boolean flags are quite common across
        // all game loops, we could abstract that out into a shared 'loop
        // context'.

        bool is_snake_one_dead = false;
        bool is_snake_two_dead = false;
        // To make the UX more forgiving, if the user is about to bump into a
        // wall we allow for a 'grace' period. This means that instead of
        // failing the game immediately, we wait for an additonal snake movement
        // tick and let the user change the direction. This requires rolling
        // back the changes that we have already applied on the head of the
        // snake and its segment body.
        bool grace_used = false;
        bool secondary_snake_grace_used = false;

        // We let the user change the new snake direction at any point during
        // the frame but it gets applied on the snake only at the end of the
        // given frame. The reason for this is that doing it each time an input
        // is registered led to issues where quick changes to the direction
        // could make the snake go opposite (turn on the spot) and fail the
        // game.
        Direction chosen_snake_direction = snake.direction;
        Direction chosen_secondary_snake_direction = secondary_snake.direction;
        int game_score = 0;
        while (!is_snake_one_dead || !is_snake_two_dead) {
                Direction dir;
                Action act;
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        if (!is_opposite(dir, snake.direction)) {
                                chosen_snake_direction = dir;
                        }
                }
                if (action_input_registered(p->action_controllers, &act)) {

                        Direction secondary_dir = action_to_direction(act);
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        if (!is_opposite(secondary_dir,
                                         secondary_snake.direction)) {
                                chosen_secondary_snake_direction =
                                    secondary_dir;
                        }
                }

                if (iteration == move_period - 1) {
                        // Process the first snake
                        if (!is_snake_one_dead)
                                take_snake_step(
                                    snake, chosen_snake_direction, &grace_used,
                                    &is_snake_one_dead, &game_score, config, gd,
                                    grid, customization, p, score_end, false);
                        if (!is_snake_two_dead)
                                take_snake_step(
                                    secondary_snake,
                                    chosen_secondary_snake_direction,
                                    &secondary_snake_grace_used,
                                    &is_snake_two_dead, &game_score, config, gd,
                                    grid, customization, p, score_end, true);
                }

                iteration += 1;
                iteration %= move_period;
                p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                p->display->refresh();
        }

        p->display->refresh();
        return UserAction::PlayAgain;
}

void take_snake_step(SnakeEntity &snake, Direction chosen_snake_direction,
                     bool *grace_used, bool *is_game_over, int *game_score,
                     SnakeDuelConfiguration &config,
                     SquareCellGridDimensions *gd,
                     std::vector<std::vector<SnakeDuelGridCell>> &grid,
                     UserInterfaceCustomization *customization, Platform *p,
                     int score_end, bool is_secondary)
{
        // TODO: helper lambdas were copied from above ->  move to peroper
        // functions
        // TODO: clean up function signature to give a more logical parameter
        // order. Those helper lambdas avoid passing the grid and display
        // parameters around each time we want to re-render a cell or draw a
        // snake segment.
        auto update_display_cell = [p, gd, &grid, customization,
                                    &config](Point *location) {
                refresh_grid_cell(p->display, customization->accent_color,
                                  config.secondary_player_color, gd, &grid,
                                  location);
        };
        auto draw_neck_and_head =
            [p, gd, &grid](Point *first_segment, Point *second_segment,
                           SnakeEntity &snake, Color color) {
                    render_segment_connection(p->display, color, gd, &grid,
                                              first_segment, second_segment);
                    render_head(p->display, color, gd, &grid, second_segment,
                                snake.direction);
            };

        snake.direction = chosen_snake_direction;

        // This modifies the snake.head in place.
        translate(&snake.head, snake.direction);

        bool hit_a_wall = is_out_of_bounds(&(snake.head), gd);
        SnakeDuelGridCell next;

        if (!hit_a_wall) {
                next = grid[snake.head.y][snake.head.x];
        }
        bool tail_bitten = next == SnakeDuelGridCell::Snake ||
                           next == SnakeDuelGridCell::SnakeWithApple ||
                           next == SnakeDuelGridCell::SecondarySnake ||
                           next == SnakeDuelGridCell::SecondarySnakeWithApple;
        bool failure = hit_a_wall || tail_bitten;

        SnakeDuelGridCell snake_body = is_secondary
                                           ? SnakeDuelGridCell::SecondarySnake
                                           : SnakeDuelGridCell::Snake;
        SnakeDuelGridCell snake_body_with_apple =
            is_secondary ? SnakeDuelGridCell::SecondarySnakeWithApple
                         : SnakeDuelGridCell::SnakeWithApple;

        if (!failure) {
                // If we got here, it means that the next cell
                // is within bounds and is not occupied by the
                // snake body. This means that we can safely
                // clear grace.
                *grace_used = false;

                // The snake has entered the next location, if
                // the next location is an apple, we mark it
                // as 'segment of snake with an apple in its
                // stomach' and render differently
                grid[snake.head.y][snake.head.x] =
                    next == SnakeDuelGridCell::Apple ? snake_body_with_apple
                                                     : snake_body;

                // We need to draw the small rectangle that
                // connects the new snake head to the rest of
                // its body.
                Point *neck = (snake.body->end() - 1).base();
                // First time a cell is drawn it happens inside
                // `draw_neck_and_head`.
                // This is needed as Snake's head needs to have
                // a different shape from all other segments.
                // Because of this, we need to update the neck
                // here to actually render it's proper contents
                // (i.e. whether it is a regular snake segment
                // or a segment that contains an apple).
                update_display_cell(neck);
                draw_neck_and_head(neck, &snake.head, snake,
                                   is_secondary ? config.secondary_player_color
                                                : customization->accent_color);
                snake.body->push_back(snake.head);

                if (next == SnakeDuelGridCell::Apple) {
                        // Eating an apple is handled by simply
                        // skipping the step where we erase the
                        // last segment of the snake (the else
                        // branch). We then spawn a new apple.
                        Point *apple_location = spawn_apple(&grid);
                        refresh_grid_cell(p->display,
                                          customization->accent_color,
                                          config.secondary_player_color, gd,
                                          &grid, apple_location);
                        (*game_score)++;
                        update_duel_score(p, gd, score_end, *game_score, is_secondary);
                } else {
                        assert(next == SnakeDuelGridCell::Empty ||
                               next == SnakeDuelGridCell::Poop);

                        // When no apple is consumed we move
                        // the snake forward by erasing its last
                        // segment. If poop functionality is
                        // enabled we leave it behind.
                        auto tail = snake.body->begin();
                        auto previous = grid[tail->y][tail->x];
                        auto updated = previous == snake_body_with_apple &&
                                               config.enable_poop
                                           ? SnakeDuelGridCell::Poop
                                           : SnakeDuelGridCell::Empty;
                        grid[tail->y][tail->x] = updated;
                        update_display_cell(tail.base());
                        snake.body->erase(tail);
                }

        } else {
                if (grace_used || !config.allow_grace) {
                        *is_game_over = true;
                }

                // We allow the user to change the
                // direction for an additional tick by
                // rolling back the head position.
                Point previous_head = *(snake.body->end() - 1);
                snake.head = {previous_head.x, previous_head.y};

                *grace_used = true;
        }
}

/**
 * Re-renders the text location above the grid informing the user about the
 * current score in the game.
 */
void update_duel_score(Platform *p, SquareCellGridDimensions *dimensions,
                       int score_text_end_location, int score, bool is_secondary)
{

        char buffer[4];
        sprintf(buffer, "%3d", score);
        // TODO: update doc
        // We start 3 letters after the end of the text end location.
        // The reason for this is that the 'Score:' text is rendered centered
        // using the render_centered_text_above_frame function and so it
        // the string is right-padded with exactly 4 spaces to make enough
        // horizontal space for the score digits to be printed (and have the
        // entire text above the grid centered). Hence we need to start
        // rendering the actual digits starting after the first space after the
        // colon in 'Score:' text, so we move the start location by 3 spaces.
        int offset = is_secondary ?  3 : 11;
        int start_position = score_text_end_location - offset * FONT_WIDTH;
        render_text_above_frame_starting_from(p, dimensions, buffer,
                                              start_position, true);
}

Point *spawn_apple(std::vector<std::vector<SnakeDuelGridCell>> *grid)
{
        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        while (true) {
                int x = rand() % cols;
                int y = rand() % rows;

                SnakeDuelGridCell selected = (*grid)[y][x];
                if (selected != SnakeDuelGridCell::Snake) {
                        (*grid)[y][x] = SnakeDuelGridCell::Apple;
                        Point *random_position = new Point();
                        random_position->x = x;
                        random_position->y = y;
                        return random_position;
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
void render_segment_connection(
    Display *display, Color snake_color, SquareCellGridDimensions *dimensions,
    std::vector<std::vector<SnakeDuelGridCell>> *grid, Point *first_location,
    Point *second_location)
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

        bool adjacent_horizontally = first_location->y == second_location->y;

        // Because of pixel-precision inaccuracies we need to make the
        // segment connections slightly wider and higher on the physical
        // display.
#ifndef EMULATOR
        int adjustment = 2;
        int height_adjustment = 2;
#else
        int adjustment = 0;
        int height_adjustment = 0;
#endif

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
                start = {.x = left_margin + (left_point.x + 1) * width -
                              adjustment,
                         .y = top_margin + left_point.y * height + padding};
                display->draw_rectangle(
                    start, padding - border_width + adjustment,
                    height - 2 * padding + height_adjustment, snake_color,
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
                                        padding - border_width + adjustment,
                                        snake_color, border_width, true);
        }
}

/**
 * Renders the head of the snake. This contains snake's eye and a rounded front
 * part of the snake.
 */
void render_head(Display *display, Color snake_color,
                 SquareCellGridDimensions *dimensions,
                 std::vector<std::vector<SnakeDuelGridCell>> *grid, Point *head,
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
        Point start = {.x = left_margin + head->x * width,
                       .y = top_margin + head->y * height};

        // We draw a 'half-cell' to connect snake head to the neck.
        int snake_w = width - 2 * padding;
        int snake_h = height - 2 * padding;
        int rectangle_w, rectangle_h;

        // Because of pixel-precision inaccuracies we need to make the
        // segment connections slightly wider and higher on the physical
        // display.
#ifndef EMULATOR
        int height_adjustment = 2;
#else
        int height_adjustment = 0;
#endif

        switch (direction) {
        case UP:
        case DOWN:
                rectangle_w = snake_w;
                rectangle_h = snake_h / 2 + height_adjustment;
                break;
        case RIGHT:
        case LEFT:
                rectangle_w = snake_w / 2;
                rectangle_h = snake_h + height_adjustment;
        }

        Point offset, eye_offset;
        switch (direction) {
        case UP:
                offset = {.x = 0, .y = snake_h / 2};
                eye_offset = {.x = -rectangle_w / 4, .y = 0};
                break;
        case LEFT:
                offset = {.x = snake_w / 2, .y = 0};
                eye_offset = {.x = 0, .y = rectangle_h / 4};
                break;
        case DOWN:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = rectangle_w / 4, .y = 0};
                break;
        case RIGHT:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = 0, .y = -rectangle_h / 4};
                break;
        }

        display->draw_rectangle({.x = start.x + offset.x + padding,
                                 .y = start.y + offset.y + padding},
                                rectangle_w, rectangle_h, snake_color,
                                border_width, true);
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
                       Color secondary_snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<SnakeDuelGridCell>> *grid,
                       Point *location)
{
        int padding = 2;
        // Because of pixel-precision inaccuracies we need to make the cells
        // slightly higher on the physical display
#ifndef EMULATOR
        int height_adjustment = 2;
#else
        int height_adjustment = 0;
#endif
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        Point grid_loc = *location;
        Point start = {.x = left_margin + grid_loc.x * width,
                       .y = top_margin + grid_loc.y * height};

        SnakeDuelGridCell cell_type = (*grid)[grid_loc.y][grid_loc.x];

        // When rendering on the actual lcd display the circle comes out a bit
        // larger because of pixel inaccuracies and internal of that lcd display
        // driver. Because of this we need to decrease the radius slightly.
        int radius_offset = 1;

        // Represents the top left corner of the snake segment
        Point padded_start = {.x = start.x + padding, .y = start.y + padding};
        Point apple_center = {.x = start.x + width / 2,
                              .y = start.y + width / 2};

        switch (cell_type) {
        case SnakeDuelGridCell::Apple: {
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Red, 0, true);
                break;
        }
        case SnakeDuelGridCell::Snake: {
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding +
                                            height_adjustment,
                                        snake_color, 1, true);
                break;
        }
        case SnakeDuelGridCell::Empty: {
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                break;
        }
        case SnakeDuelGridCell::SnakeWithApple: {
                // Here we render a normal snake segment that has a hole inside
                // of it with an apple sitting there. This is to indicate
                // segments of the snake that have 'consumed an apple'
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding +
                                            height_adjustment,
                                        snake_color, 1, true);
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Black, 0, true);
                display->draw_circle(
                    apple_center, (width - 2 * padding) / 2 - 2 * radius_offset,
                    Color::Red, 0, true);
                break;
        }
        case SnakeDuelGridCell::Poop: {

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
                // TODO: remove duplication with normal snake rendering.
        case SnakeDuelGridCell::SecondarySnake:
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding +
                                            height_adjustment,
                                        secondary_snake_color, 1, true);
                break;
        case SnakeDuelGridCell::SecondarySnakeWithApple:
                // Here we render a normal snake segment that has a hole inside
                // of it with an apple sitting there. This is to indicate
                // segments of the snake that have 'consumed an apple'
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding +
                                            height_adjustment,
                                        secondary_snake_color, 1, true);
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Black, 0, true);
                display->draw_circle(
                    apple_center, (width - 2 * padding) / 2 - 2 * radius_offset,
                    Color::Red, 0, true);
                break;
                break;
        }
}

Configuration *assemble_snake_duel_configuration(PersistentStorage *storage);
void extract_game_config(SnakeDuelConfiguration *game_config,
                         Configuration *config);
std::optional<UserAction>
collect_snake_duel_config(Platform *p, SnakeDuelConfiguration *game_config,
                          UserInterfaceCustomization *customization)
{
        Configuration *config =
            assemble_snake_duel_configuration(p->persistent_storage);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        free_configuration(config);
        return std::nullopt;
}

SnakeDuelConfiguration *
load_initial_snake_duel_config(PersistentStorage *storage);
Configuration *assemble_snake_duel_configuration(PersistentStorage *storage)
{
        SnakeDuelConfiguration *initial_config =
            load_initial_snake_duel_config(storage);

        ConfigurationOption *speed = ConfigurationOption::of_integers(
            "Speed", {4, 5, 6, 7, 8}, initial_config->speed);

        auto *poop = ConfigurationOption::of_strings(
            "Poop", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->enable_poop));

        auto *allow_grace = ConfigurationOption::of_strings(
            "Grace", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_grace));

        auto *secondary_player_color = ConfigurationOption::of_colors(
            "Color", AVAILABLE_COLORS, initial_config->secondary_player_color);

        std::vector<ConfigurationOption *> options = {speed, poop, allow_grace,
                                                      secondary_player_color};

        return new Configuration("Snake Duel", options, "Start Game");
}

SnakeDuelConfiguration *
load_initial_snake_duel_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offsets()[SnakeDuel];
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        // We initialize empty config to detect corrupted memory and fallback
        // to defaults if needed.
        SnakeDuelConfiguration config = {.speed = 0,
                                         .allow_grace = 0,
                                         .enable_poop = false,
                                         .secondary_player_color = Blue};

        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage->get(storage_offset, config);

        SnakeDuelConfiguration *output = new SnakeDuelConfiguration();

        if (config.speed == 0) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "snake configuration, using default values.");
                memcpy(output, &DEFAULT_SNAKE_DUEL_CONFIG,
                       sizeof(SnakeDuelConfiguration));
                storage->put(storage_offset, DEFAULT_SNAKE_DUEL_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(SnakeDuelConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded snake configuration: speed=%d, enable_poop=%d, "
                  "allow_grace=%d"
                  "secondary_player_color=%d",
                  output->speed, output->enable_poop, output->allow_grace,
                  output->secondary_player_color);

        return output;
}

void extract_game_config(SnakeDuelConfiguration *game_config,
                         Configuration *config)
{
        ConfigurationOption speed = *config->options[0];
        ConfigurationOption enable_poop = *config->options[1];
        ConfigurationOption allow_grace = *config->options[2];
        ConfigurationOption secondary_player_color = *config->options[3];

        auto yes_or_no_option_to_bool = [](ConfigurationOption option) {
                return extract_yes_or_no_option(option.get_current_str_value());
        };

        game_config->speed = speed.get_curr_int_value();
        game_config->enable_poop = yes_or_no_option_to_bool(enable_poop);
        game_config->allow_grace = yes_or_no_option_to_bool(allow_grace);
        game_config->secondary_player_color =
            secondary_player_color.get_current_color_value();
}
