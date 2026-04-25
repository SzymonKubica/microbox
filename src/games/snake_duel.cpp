#include <cassert>
#include <cstring>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "snake_common.hpp"
#include "snake_duel.hpp"
#include "settings.hpp"
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/profiling.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50
#define TAG "snake"

constexpr int GAME_CELL_WIDTH = DEFAULT_SNAKE_GAME_CELL_WIDTH;

SnakeDuelConfiguration DEFAULT_SNAKE_DUEL_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1},
    .speed = 6,
    .allow_grace = false,
    .enable_poop = true,
    .secondary_player_color = Blue};

using namespace SnakeDefinitions;

struct ColoredSnake : Snake {
        Color color;

        ColoredSnake(Point head_position, Direction initial_direction,
                     Color snake_color)
            : Snake(head_position, initial_direction), color(snake_color)
        {
        }
};

/**
 * Structure bundling up all flags / counters that are required to manage the
 * state of an ongoing game loop.
 */
struct SnakeDuelLoopState {
        int frame_duration;
        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'pause' button for too long and
        // the game stutters instead of being properly paused.
        bool action_input_on_last_iteration;
        // To make the UX more forgiving, if the user is about to bump into a
        // wall we allow for a 'grace' period. This means that instead of
        // failing the game immediately, we wait for an additonal snake movement
        // tick and let the user change the direction. This requires rolling
        // back the changes that we have already applied on the head of the
        // snake and its segment body.
        bool grace_used;
        bool second_snake_grace_used;
        bool is_paused;

        bool is_snake_one_dead;
        bool is_snake_two_dead;

        int snake_one_score;
        int snake_two_score;

        long last_step_timestamp;

      public:
        SnakeDuelLoopState(int moves_per_second)
            : action_input_on_last_iteration(false), is_snake_two_dead(false),
              is_snake_one_dead(false), grace_used(false),
              second_snake_grace_used(false), is_paused(false),
              snake_one_score(0), snake_two_score(0)
        {
                this->frame_duration = (1000 / moves_per_second);
                this->last_step_timestamp = 0;
        }

        bool is_game_over() { return is_snake_one_dead && is_snake_two_dead; }

        /*
         * Informs us whether a sufficient number of waiting iterations has
         * passed to take a game loop step (e.g. advance the snake by one unit
         * forward).
         */
        bool is_waiting(TimeProvider *time)
        {
                return time->milliseconds() - last_step_timestamp <
                       frame_duration;
        }
};

UserAction snake_duel_loop(Platform *p,
                           UserInterfaceCustomization *customization);

const char *SnakeDuel::get_game_name() { return "Snake Duel"; }
const char *SnakeDuel::get_help_text()
{
        return "Use the joystick to control where the snake goes."
               "Consume apples to grow the snake. Avoid hitting the walls or "
               "snake's tail. Second player: use keypad to control the snake.";
}

void update_duel_score(Platform *p, SquareCellGridDimensions *dimensions,
                       int score_text_end_location, int score,
                       bool is_secondary = false);

/**
 * Combines `find_next_step_towards_apple` and `find_fallback_next_safe_step`
 * to steer the 'AI' snake.
 */
std::optional<Direction> next_step(Snake &snake,
                                   std::vector<std::vector<Cell>> &grid,
                                   SquareCellGridDimensions *gd);
/**
 * Given the snake (its current head location) and the current state of the
 * grid it finds the direction where the snake needs to turn in this moment
 * to get too the apple.
 *
 * This works by finding the path to the apple and then looking at the
 * displacement vector between the current location and the next step on the
 * path towards the apple.
 *
 * Uses DFS with a heuristic to traverse nodes that are closer to the apple with
 * respect to manhattan distance.
 */
std::optional<Direction>
find_next_step_towards_apple(Snake &snake,
                             std::vector<std::vector<Cell>> &grid);

std::optional<Direction>
find_fallback_next_safe_step(Snake &snake, std::vector<std::vector<Cell>> &grid,
                             SquareCellGridDimensions *gd);
void take_snake_step(
    Platform *p, UserInterfaceCustomization *customization,
    const SnakeDuelConfiguration &config, SquareCellGridDimensions *gd,
    int score_text_end_x, std::vector<std::vector<Cell>> &grid,
    std::function<void(ColoredSnake &snake)> &render_head,
    std::function<void(Point &point, Color color)> &render_cell,
    SnakeDuelLoopState &state, ColoredSnake &snake, bool is_secondary);

UserAction SnakeDuel::app_loop(Platform *p,
                               UserInterfaceCustomization *customization,
                               const SnakeDuelConfiguration &config)
{
        LOG_DEBUG(TAG, "Entering Snake game loop");

        int game_cell_width = DEFAULT_SNAKE_GAME_CELL_WIDTH;
        std::unique_ptr<SquareCellGridDimensions> gd(calculate_grid_dimensions(
            p->display->get_width(), p->display->get_height(),
            p->display->get_display_corner_radius(), game_cell_width));
        int rows = gd->rows;
        int cols = gd->cols;

        draw_grid_frame(p, customization, gd.get());

        std::vector<std::vector<Cell>> grid(rows, std::vector<Cell>(cols));

        // We render the 'Score:' text only once but including the empty space
        // required for the score. This is needed to ensure that the score
        // section above the grid is properly centered. When rendering the
        // actual score count, we need to cancel out the three spaces and
        // subtract them from score_end pixel position.
        int score_end =
            render_centered_above_frame(p, gd.get(), (char *)"P1:    P2:    ");

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
        // Re-renders the score of Player 1 in the correct location above the
        // grid as determined by grid dimensions and the score text end x
        // position.
        auto render_player_1_score = [p, &gd, &grid, score_end](int score) {
                update_duel_score(p, gd.get(), score_end, score);
        };
        // Re-renders the score of Player 2 in the correct location above the
        // grid as determined by grid dimensions and the score text end x
        // position.
        auto render_player_2_score = [p, &gd, &grid, score_end](int score) {
                update_duel_score(p, gd.get(), score_end, score, true);
        };
        // After the value of a given cell in the grid is changed, this
        // re-renders that single cell in the display.
        std::function<void(Point & location, Color color)> render_cell =
            [p, &gd, &grid, customization](Point &location, Color color) {
                    refresh_grid_cell(p->display, color, gd.get(), &grid,
                                      location);
            };
        // Renders the snake's head including the neck (2nd segment right behind
        // the head).
        std::function<void(ColoredSnake & snake)> render_head =
            [p, &gd, &grid, customization](ColoredSnake &snake) {
                    auto neck = snake.get_neck();
                    render_segment_connection(p->display, snake.color, gd.get(),
                                              &grid, neck, snake.head);
                    render_snake_head(p->display, snake.color, gd.get(), &grid,
                                      snake);
            };

        render_player_1_score(0);
        render_player_2_score(0);

        if (!p->display->refresh()) {
                return UserAction::CloseWindow;
        }

        auto primary_color = customization->accent_color;
        auto secondary_color = config.secondary_player_color;

        int mid_x = cols / 2;
        int mid_y = rows / 2;
        Point midpoint = {mid_x, mid_y};

        // The first snake starts in the middle pointing to the right.
        // The second snake is one cell below in the opposite direction.
        ColoredSnake snake = {midpoint, Direction::RIGHT, primary_color};
        ColoredSnake second_snake = {translate_pure(midpoint, Direction::DOWN),
                                     Direction::LEFT, secondary_color};

        set_cell(snake.head, Cell::Snake);
        set_cell(snake.tail, Cell::Snake);
        set_cell(second_snake.head, Cell::Snake);
        set_cell(second_snake.tail, Cell::Snake);

        // First render of the first snake
        render_cell(snake.tail, snake.color);
        render_head(snake);
        render_cell(snake.head, snake.color);

        // First render of the second snake
        render_cell(second_snake.tail, second_snake.color);
        render_head(second_snake);
        render_cell(second_snake.head, second_snake.color);

        Point apple_location = spawn_apple(&grid);

        // Here the color doesn't matter as apples are always red.
        render_cell(apple_location, primary_color);

        /*
         Time-related shorthand functions
         */
        auto current_time = [&]() { return p->time_provider->milliseconds(); };
        auto delay_millis = [&](int duration_millis) {
                p->time_provider->delay_ms(duration_millis);
        };
        SnakeDuelLoopState state{config.speed};

        // Convenience funtion to ensure each short-circuit exit of the
        // loop iteration waits until the next move.
        auto wait_for_next_move =
            [p, &state, &current_time, &delay_millis](
                long frame_start_millis) -> std::optional<UserAction> {
                long elapsed = current_time() - frame_start_millis;
                if (GAME_LOOP_DELAY > elapsed)
                        delay_millis(GAME_LOOP_DELAY - elapsed);
                if (!p->display->refresh())
                        return UserAction::CloseWindow;
                return std::nullopt;
        };

        // We let the user change the new snake direction at any point
        // during the frame but it gets applied on the snake only at the
        // end of the given frame. The reason for this is that doing it
        // each time an input is registered led to issues where quick
        // changes to the direction could make the snake go opposite
        // (turn on the spot) and fail the game.
        Direction new_snake_direction = snake.direction;
        Direction new_second_snake_direction = second_snake.direction;
        while (!state.is_game_over()) {
                long frame_start = current_time();
                Direction dir;
                Action act;
                // The `!is_opposite` check prevents instant game-over when user
                // presses the direction that is opposite to the current
                // direction of the snake.
                if (poll_directional_input(p->directional_controllers, &dir)) {
                        if (!is_opposite(dir, snake.direction)) {
                                new_snake_direction = dir;
                        }
                }
                if (poll_action_input(p->action_controllers, &act)) {
                        Direction second_dir = action_to_direction(act);
                        if (!is_opposite(second_dir, second_snake.direction)) {
                                new_second_snake_direction = second_dir;
                        }
                        // If the player 1 is dead and we are running in the AI
                        // mode, we let the player quit early by pressing blue.
                        if (config.enable_ai && state.is_snake_one_dead &&
                            act == Action::BLUE) {
                                delay_millis(MOVE_REGISTERED_DELAY);
                                return UserAction::PauseAndPlayAgain;
                        }
                }

                // If we are paused or it is not the time to move yet, we finish
                // processing early.
                if (state.is_waiting(p->time_provider)) {
                        if (wait_for_next_move(frame_start).has_value())
                                return UserAction::CloseWindow;
                        continue;
                }
                state.last_step_timestamp = current_time();

                TimeProvider *timer = p->time_provider;
                {
                        // Logs the elapsed time upon destruction at the end of
                        // the enclosing block.
                        DurationLogger l(timer, "Snake 1 moved in %d millis.");

                        if (!state.is_snake_one_dead) {
                                snake.direction = new_snake_direction;
                                take_snake_step(p, customization, config,
                                                gd.get(), score_end, grid,
                                                render_head, render_cell, state,
                                                snake, false);
                        }
                }
                {
                        DurationLogger l(timer, "Snake 2 moved in %d millis.");

                        if (!state.is_snake_two_dead) {
                                if (config.enable_ai) {
                                        auto maybe_next_step = next_step(
                                            second_snake, grid, gd.get());
                                        if (maybe_next_step.has_value()) {
                                                new_second_snake_direction =
                                                    maybe_next_step.value();
                                        }
                                }

                                second_snake.direction =
                                    new_second_snake_direction;
                                take_snake_step(p, customization, config,
                                                gd.get(), score_end, grid,
                                                render_head, render_cell, state,
                                                second_snake, true);
                        }
                }

                if (wait_for_next_move(frame_start).has_value()) {
                        return UserAction::CloseWindow;
                };
                int elapsed = current_time() - frame_start;
                LOG_DEBUG(TAG, "Iteration took %d milliseconds.", elapsed);
        }

        if (!p->display->refresh()) {
                return UserAction::CloseWindow;
        }
        return UserAction::PauseAndPlayAgain;
}

void take_snake_step(
    Platform *p, UserInterfaceCustomization *customization,
    const SnakeDuelConfiguration &config, SquareCellGridDimensions *gd,
    int score_text_end_x, std::vector<std::vector<Cell>> &grid,
    std::function<void(ColoredSnake &snake)> &render_head,
    std::function<void(Point &point, Color color)> &render_cell,
    SnakeDuelLoopState &state, ColoredSnake &snake, bool is_secondary)
{

        // Performs a lookup of the grid value without explicit array indexing.
        auto get_cell = [&grid](const Point &location) {
                return grid[location.y][location.x];
        };

        // We neeed to resolve state properties owned by this snake.
        bool *grace_used;
        int *game_score;
        bool *is_game_over;
        int snake_number;
        if (is_secondary) {
                grace_used = &state.second_snake_grace_used;
                game_score = &state.snake_two_score;
                is_game_over = &state.is_snake_two_dead;
                snake_number = 2;
        } else {
                grace_used = &state.grace_used;
                game_score = &state.snake_one_score;
                is_game_over = &state.is_snake_one_dead;
                snake_number = 1;
        }

        // This modifies the snake.head in place.
        translate(&snake.head, snake.direction);

        bool wall_hit = is_out_of_bounds(&(snake.head), gd);
        Cell next;

        if (!wall_hit) {
                next = get_cell(snake.head);
        }
        bool tail_hit = next == Cell::Snake || next == Cell::AppleSnake;
        bool failure = wall_hit || tail_hit;

        if (failure) {
                if (*grace_used || !config.allow_grace) {
                        LOG_INFO(TAG, "Snake %d is dead.", snake_number);
                        *is_game_over = true;
                }

                // We allow the user to change the direction for an additional
                // tick by rolling back the head position.
                LOG_INFO(TAG, "Snake %d used grace.", snake_number);
                Point previous_head = *(snake.body.end() - 1);
                snake.head = {previous_head.x, previous_head.y};

                *grace_used = true;
                return;
        }

        // If we got here, the next cell is within bounds and is not occupied by
        // the snake body. This means that we can safely clear grace.
        *grace_used = false;

        // The snake has entered the next location, if the next location is an
        // apple, we mark it as 'segment of snake with an apple in its stomach'
        // and that gets rendered differently
        grid[snake.head.y][snake.head.x] =
            next == Cell::Apple ? Cell::AppleSnake : Cell::Snake;

        // We need to draw the small rectangle that connects the new snake head
        // to the rest of its body. This is needed as Snake's head needs to
        // have a different shape from all other segments. Because of this, we
        // need to update the neck here to actually render it's proper contents
        // (i.e. whether it is a regular snake segment or a segment that
        // contains an apple).
        snake.body.push_back(snake.head);
        auto neck = snake.get_neck();
        render_cell(neck, snake.color);
        render_head(snake);

        if (next == Cell::Apple) {
                // Eating an apple is handled by simply skipping the step where
                // we erase the last segment of the snake (the else branch). We
                // then spawn a new apple.
                Point apple_location = spawn_apple(&grid);
                // Here the color doesn't matter as apples are always red.
                render_cell(apple_location, snake.color);
                (*game_score)++;
                update_duel_score(p, gd, score_text_end_x, *game_score,
                                  is_secondary);
                return;
        }

        assert(next == Cell::Empty || next == Cell::Poop);

        // When no apple is consumed we move the snake forward by erasing its
        // last segment. If poop functionality is enabled we leave it behind.
        auto tail_iter = snake.body.begin();
        auto tail = *tail_iter;

        Cell updated = Cell::Empty;
        // If the last segment contains an apple and poop visuals are enabled,
        // we leave poop behind the snake.
        if (config.enable_poop && get_cell(tail) == Cell::AppleSnake) {
                updated = Cell::Poop;
        }
        grid[tail_iter->y][tail_iter->x] = updated;
        render_cell(tail, snake.color);
        snake.body.erase(tail_iter);
}

/**
 * Re-renders the text location above the grid informing the user about the
 * current score in the game.
 *
 * @param is_secondary If set, the score of the second player (P2) will be
 * updated.
 */
void update_duel_score(Platform *p, SquareCellGridDimensions *dimensions,
                       int score_text_end_location, int score,
                       bool is_secondary)
{

        char buffer[4];
        sprintf(buffer, "%3d", score);
        // We start n letters after the end of the text end location.
        // The reason for this is that the 'P1:' or 'P2' text is rendered
        // centered using:
        // int score_end =
        // render_centered_text_above_frame(p, gd, (char *)"P1:    P2:    ");
        // it the template is interspersed with 4 empty spaces to make enough
        // horizontal space for the score digits. Hence we need to start
        // rendering the actual digits starting after the first space after the
        // colon in 'P1:' text, so we move the start location by 3 spaces. If we
        // are updating the score of the second player, we need to set the
        // offset as 11 to start writing after the 'P2:' text.
        int offset = is_secondary ? 3 : 11;
        int start_position = score_text_end_location - offset * FONT_WIDTH;
        render_text_above_frame_starting_from(p, dimensions, buffer,
                                              start_position, true);
}

/**
 * Forward declarations of functions related to configuration manipulation.
 */
Configuration *assemble_snake_duel_configuration(PersistentStorage *storage);
void extract_game_config(SnakeDuelConfiguration *game_config,
                         Configuration *config);
SnakeDuelConfiguration *
load_initial_snake_duel_config(PersistentStorage *storage);

std::optional<UserAction>
SnakeDuel::collect_config(Platform *p,
                          UserInterfaceCustomization *customization,
                          SnakeDuelConfiguration *game_config)
{
        Configuration *config =
            assemble_snake_duel_configuration(p->persistent_storage);

        auto maybe_interrupt = collect_configuration(p, config, customization);
        if (maybe_interrupt) {
                delete config;
                return maybe_interrupt;
        }

        extract_game_config(game_config, config);
        delete config;
        return std::nullopt;
}

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

        auto *ai_mode = ConfigurationOption::of_strings(
            "AI", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->enable_ai));

        auto *secondary_player_color = ConfigurationOption::of_colors(
            "Color", AVAILABLE_COLORS, initial_config->secondary_player_color);

        std::vector<ConfigurationOption *> options = {
            speed, poop, allow_grace, ai_mode, secondary_player_color};

        delete initial_config;
        return new Configuration("Snake Duel", options);
}

SnakeDuelConfiguration *
load_initial_snake_duel_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::SnakeDuel);
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        SnakeDuelConfiguration config;

        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage->get(storage_offset, config);

        SnakeDuelConfiguration *output = new SnakeDuelConfiguration();

        if (!config.header.is_valid()) {
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
                  "allow_grace=%d, "
                  "secondary_player_color=%d",
                  output->speed, output->enable_poop, output->allow_grace,
                  output->secondary_player_color);

        return output;
}

void extract_game_config(SnakeDuelConfiguration *game_config,
                         Configuration *config)
{
        // It is important that we don't copy any of the ConfigurationOption as
        // they internally contain pointers to their option values and running
        // destructor at the end would result in a double-free.
        ConfigurationOption *speed = config->options[0];
        ConfigurationOption *enable_poop = config->options[1];
        ConfigurationOption *allow_grace = config->options[2];
        ConfigurationOption *enable_ai = config->options[3];
        ConfigurationOption *secondary_player_color = config->options[4];

        auto yes_or_no_option_to_bool = [](ConfigurationOption *option) {
                return extract_yes_or_no_option(
                    option->get_current_str_value());
        };

        game_config->speed = speed->get_curr_int_value();
        game_config->enable_poop = yes_or_no_option_to_bool(enable_poop);
        game_config->allow_grace = yes_or_no_option_to_bool(allow_grace);
        game_config->enable_ai = yes_or_no_option_to_bool(enable_ai);
        game_config->secondary_player_color =
            secondary_player_color->get_current_color_value();
}

/* Functions responsible for 'AI' snake steering follow below */

std::optional<Direction> next_step(Snake &snake,
                                   std::vector<std::vector<Cell>> &grid,
                                   SquareCellGridDimensions *gd);

/**
 * Given the current position of the snake, and the grid with all currently
 * occupied cells and apple location, it finds a path to the apple using BFS
 * and then sets the next step along that path in the &next output
 * parameter.
 *
 * The reason we are doing this in such an 'impure' way is that returning
 * the entire path works on a powerful desktop machine, however on an
 * arduino we would get crashes which I suspect were memory related.
 */
bool find_next_step(const Point &start,
                    const std::vector<std::vector<Cell>> &grid,
                    Point &next_step);

std::optional<Direction>
find_next_step_towards_apple(Snake &snake, std::vector<std::vector<Cell>> &grid)
{

        Point curr = snake.head;
        Point next;
        if (!find_next_step(curr, grid, next)) {
                return std::nullopt;
        };

        LOG_DEBUG(TAG, "Next location: {x: %d, y: %d}", next.x, next.y);
        LOG_DEBUG(TAG, "Head: {x: %d, y: %d}", snake.head.x, snake.head.y);

        return determine_displacement_direction(snake.head, next);
}

/**
 * If it is not possible to find the next step towards apple (e.g. the path does
 * not exist at the moment as the snake tail blocks it), we don't want the AI
 * snake to crash into the wall. Instead we need to find the next direction that
 * is safe to take. This is determined by looking at the neighbours of the
 * current location and picking the direction where the next cell is accessible.
 */
std::optional<Direction>
find_fallback_next_safe_step(Snake &snake, std::vector<std::vector<Cell>> &grid,
                             SquareCellGridDimensions *gd)
{

        int rows = grid.size();
        int cols = grid[0].size();

        Point curr = snake.head;
        Point next = translate_pure(curr, snake.direction);

        auto is_accessible = [&grid](Point p) {
                return grid[p.y][p.x] != Cell::Snake &&
                       grid[p.y][p.x] != Cell::AppleSnake;
        };

        // Simple case: the cell ahead is accessible
        if (!is_out_of_bounds(&next, gd) && is_accessible(next)) {
                return snake.direction;
        }

        auto neighbours =
            get_adjacent_neighbours_inside_grid(&curr, rows, cols);

        for (const auto &nb : neighbours) {
                if (is_accessible(nb)) {
                        next = nb;
                        break;
                }
        }

        return determine_displacement_direction(snake.head, next);
}

std::optional<Direction> next_step(Snake &snake,
                                   std::vector<std::vector<Cell>> &grid,
                                   SquareCellGridDimensions *gd)
{
        auto direction = find_next_step_towards_apple(snake, grid);
        if (direction.has_value() &&
            !is_opposite(direction.value(), snake.direction)) {
                return direction.value();
        }

        auto fallback = find_fallback_next_safe_step(snake, grid, gd);
        // Fallback will never turn backwards in place. Hence, no need to check
        // if the fallback direction is opposite.
        if (fallback.has_value()) {
                return fallback.value();
        }
        return std::nullopt;
}

/**
 * For large statically-allocated arrays of points we want to minimize the
 * global variable footprint. We use this 'compact' point that is optimized
 * based on the assumption that the x coordinate is within the [0, 23] range
 * and y is within [0,19], because of this we need 5 bits to encode each of the
 * coordinates.
 */
typedef struct CompactPoint {
        uint8_t x;
        uint8_t y;

      public:
        Point into_point() { return {x, y}; }
        static CompactPoint from_point(const Point &point)
        {
                return {static_cast<uint8_t>(point.x),
                        static_cast<uint8_t>(point.y)};
        }

} CompactPoint;

/*
 * To optimize memory usage and avoid fragmentation we statically preallocate
 * all arrays that are used for tracking the state of the traversal.
 */

// Compile-time calculation of the number of grid columns.
constexpr int MAX_COLS =
    grid_max_cols(DISPLAY_WIDTH, DISPLAY_CORNER_RADIUS, GAME_CELL_WIDTH);
// Compile-time calculation of the number of grid rows.
constexpr int MAX_ROWS =
    grid_max_rows(DISPLAY_HEIGHT, DISPLAY_CORNER_RADIUS, GAME_CELL_WIDTH);
// Each row is up to 24 cells long, hence we can represent whether a cell was
// visited by setting bits.
static uint32_t visited[MAX_ROWS];

static CompactPoint parent[MAX_ROWS][MAX_COLS];
static CompactPoint queue[MAX_ROWS * MAX_COLS];

bool find_next_step(const Point &start,
                    const std::vector<std::vector<Cell>> &grid,
                    Point &next_step)
{

        auto is_visited = [](Point point) -> bool {
                return static_cast<bool>(
                    visited[point.y] & (1u << static_cast<uint32_t>(point.x)));
        };
        auto set_visited = [](Point point) {
                visited[point.y] |= (1u << static_cast<uint32_t>(point.x));
        };

        int rows = grid.size();
        int cols = grid[0].size();
        Point apple;
        // Mark all cells where we cannot go and find the apple.
        for (int y = 0; y < rows; ++y) {
                // Clear the previous state of visited cells
                visited[y] = 0;
                for (int x = 0; x < cols; ++x) {
                        // Clear the parent map
                        parent[y][x] = {0, 0};

                        // Assign initial state depending on the grid
                        if (grid[y][x] == Cell::Apple) {
                                apple = {x, y};
                                continue;
                        }
                        Cell curr = grid[y][x];
                        if (curr != Cell::Empty) {
                                set_visited({x, y});
                        }
                }
        }

        int queue_head_idx = 0;
        int queue_tail_idx = 0;
        auto enqueue = [&queue_tail_idx](Point next) {
                queue[queue_tail_idx++] = CompactPoint::from_point(next);
        };
        auto dequeue = [&queue_head_idx]() -> Point {
                return queue[queue_head_idx++].into_point();
        };

        enqueue(start);
        set_visited(start);

        while (queue_head_idx != queue_tail_idx) {
                Point cur = dequeue();

                if (cur.x == apple.x && cur.y == apple.y) {
                        // Traverse back the parent map to only get the next
                        // step.
                        Point p = apple;
                        while (!(parent[p.y][p.x].x == start.x &&
                                 parent[p.y][p.x].y == start.y)) {
                                p = parent[p.y][p.x].into_point();
                        }
                        next_step = p;
                        return true;
                }

                auto neighbours =
                    get_adjacent_neighbours_inside_grid(&cur, rows, cols);

                for (auto &nb : neighbours) {
                        if (is_visited(nb))
                                continue;

                        set_visited(nb);
                        parent[nb.y][nb.x] = CompactPoint::from_point(cur);
                        enqueue(nb);
                }
        }

        // If we didn't find the path that does not touch snake poop, we
        // try again but now we allow for stepping on it. The distinction here
        // is made that we only mark Snake/AppleSnake cells as inaccessible.
        for (int y = 0; y < rows; ++y) {
                // Clear the previous state of visited cells
                visited[y] = 0;
                for (int x = 0; x < cols; ++x) {
                        // Clear the parent map
                        parent[y][x] = {0, 0};

                        Cell curr = grid[y][x];
                        if (curr == Cell::Snake || curr == Cell::AppleSnake) {
                                set_visited({x, y});
                        }
                }
        }

        queue_head_idx = 0;
        queue_tail_idx = 0;
        enqueue(start);

        while (queue_head_idx != queue_tail_idx) {
                Point cur = dequeue();

                if (cur.x == apple.x && cur.y == apple.y) {
                        // Traverse back the parent map to only get the next
                        // step.
                        Point p = apple;
                        while (!(parent[p.y][p.x].x == start.x &&
                                 parent[p.y][p.x].y == start.y)) {
                                p = parent[p.y][p.x].into_point();
                        }
                        next_step = p;
                        return true;
                }

                auto neighbours =
                    get_adjacent_neighbours_inside_grid(&cur, rows, cols);

                for (auto &nb : neighbours) {
                        if (is_visited(nb))
                                continue;

                        set_visited(nb);
                        parent[nb.y][nb.x] = CompactPoint::from_point(cur);
                        enqueue(nb);
                }
        }

        return false;
}
