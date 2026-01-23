#include "snake_common.hpp"
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

using namespace SnakeDefinitions;

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

void update_duel_score(Platform *p, SquareCellGridDimensions *dimensions,
                       int score_text_end_location, int score,
                       bool is_secondary = false);

void take_snake_step(Snake &snake, Direction chosen_snake_direction,
                     bool *grace_used, bool *is_game_over, int *game_score,
                     SnakeDuelConfiguration &config,
                     SquareCellGridDimensions *gd,
                     std::vector<std::vector<Cell>> &grid,
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

        LOG_DEBUG(TAG, "Rendering snake duel game area.");
        draw_grid_frame(p, customization, gd);

        std::vector<std::vector<Cell>> grid(rows, std::vector<Cell>(cols));

        // We render the 'Score:' text only once but including the empty space
        // required for the score. This is needed to ensure that the score
        // section above the grid is properly centered. When rendering the
        // actual score count, we need to cancel out the three spaces and
        // subtract them from score_end pixel position.
        int score_end =
            render_centered_above_frame(p, gd, (char *)"P1:    P2:    ");

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
        auto render_player_1_score = [p, gd, &grid, score_end](int score) {
                update_duel_score(p, gd, score_end, score);
        };
        // Re-renders the score of Player 1 in the correct location above the
        // grid as determined by grid dimensions and the score text end x
        // position.
        auto render_player_2_score = [p, gd, &grid, score_end](int score) {
                update_duel_score(p, gd, score_end, score, true);
        };
        // After the value of a given cell in the grid is changed, this
        // re-renders that single cell in the display.
        auto render_cell = [p, gd, &grid, customization](Point &location) {
                refresh_grid_cell(p->display, customization->accent_color, gd,
                                  &grid, location);
        };

        LOG_DEBUG(TAG, "Snake game area border drawn.");

        render_player_1_score(0);
        render_player_2_score(0);

        p->display->refresh();

        // The first snake starts in the middle of the game area pointing to the
        // right.
        Point snake_head = {.x = cols / 2, .y = rows / 2};
        Point snake_tail = {.x = snake_head.x - 1, .y = snake_head.y};
        Snake snake = {snake_head, Direction::RIGHT};
        set_cell(snake.head, Cell::Snake);
        set_cell(snake.tail, Cell::Snake);

        // The first snake starts one cell below the first one going in the
        // opposite direction
        Point secondary_snake_head = {.x = cols / 2, .y = rows / 2 + 1};
        Snake secondary_snake = {secondary_snake_head, Direction::LEFT};

        set_cell(secondary_snake.head, Cell::Snake);
        set_cell(secondary_snake.tail, Cell::Snake);

        // Those helper lambdas avoid passing the grid and display parameters
        // around each time we want to re-render a cell or draw a snake segment.
        auto update_display_cell = [p, gd, &grid, customization,
                                    &config](Point &location, Color color) {
                refresh_grid_cell(p->display, color, gd, &grid, location);
        };
        auto draw_neck_and_head = [p, gd, &grid](Point first_segment,
                                                 Point second_segment,
                                                 Snake &snake, Color color) {
                render_segment_connection(p->display, color, gd, &grid,
                                          first_segment, second_segment);
                render_snake_head(p->display, color, gd, &grid, snake);
        };

        auto primary_color = customization->accent_color;
        auto secondary_color = config.secondary_player_color;
        // First render of the first snake
        update_display_cell(snake_tail, primary_color);
        draw_neck_and_head(snake_tail, snake_head, snake, primary_color);
        update_display_cell(snake_head, primary_color);

        // First render of the second snake
        update_display_cell(secondary_snake.tail, secondary_color);
        draw_neck_and_head(secondary_snake.tail, secondary_snake.head,
                           secondary_snake, secondary_color);
        update_display_cell(secondary_snake_head, secondary_color);

        Point apple_location = spawn_apple(&grid);
        // Here the color doesn't matter as apples are always red.
        update_display_cell(apple_location, primary_color);

        int move_period = (1000 / config.speed) / GAME_LOOP_DELAY;
        int iteration = 0;

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
                if (poll_directional_input(p->directional_controllers, &dir)) {
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        if (!is_opposite(dir, snake.direction)) {
                                chosen_snake_direction = dir;
                        }
                }
                if (poll_action_input(p->action_controllers, &act)) {

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

void take_snake_step(Snake &snake, Direction chosen_snake_direction,
                     bool *grace_used, bool *is_game_over, int *game_score,
                     SnakeDuelConfiguration &config,
                     SquareCellGridDimensions *gd,
                     std::vector<std::vector<Cell>> &grid,
                     UserInterfaceCustomization *customization, Platform *p,
                     int score_end, bool is_secondary)
{
        // TODO: helper lambdas were copied from above ->  move to proper
        // functions
        // TODO: clean up function signature to give a more logical parameter
        // order. Those helper lambdas avoid passing the grid and display
        // parameters around each time we want to re-render a cell or draw a
        // snake segment.
        auto color = is_secondary ? config.secondary_player_color
                                  : customization->accent_color;
        auto update_display_cell = [p, gd, &grid, customization, &config,
                                    color](Point &location) {
                refresh_grid_cell(p->display, color, gd, &grid, location);
        };
        auto draw_neck_and_head = [p, gd, &grid, color](Point first_segment,
                                                        Point second_segment,
                                                        const Snake &snake) {
                render_segment_connection(p->display, color, gd, &grid,
                                          first_segment, second_segment);
                render_snake_head(p->display, color, gd, &grid, snake);
        };

        snake.direction = chosen_snake_direction;

        // This modifies the snake.head in place.
        translate(&snake.head, snake.direction);

        bool hit_a_wall = is_out_of_bounds(&(snake.head), gd);
        Cell next;

        if (!hit_a_wall) {
                next = grid[snake.head.y][snake.head.x];
        }
        bool tail_bitten = next == Cell::Snake || next == Cell::AppleSnake;
        bool failure = hit_a_wall || tail_bitten;

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
                    next == Cell::Apple ? Cell::AppleSnake : Cell::Snake;

                // We need to draw the small rectangle that
                // connects the new snake head to the rest of
                // its body.
                Point *neck = (snake.body.end() - 1).base();
                // First time a cell is drawn it happens inside
                // `draw_neck_and_head`.
                // This is needed as Snake's head needs to have
                // a different shape from all other segments.
                // Because of this, we need to update the neck
                // here to actually render it's proper contents
                // (i.e. whether it is a regular snake segment
                // or a segment that contains an apple).
                update_display_cell(*neck);
                draw_neck_and_head(*neck, snake.head, snake);
                snake.body.push_back(snake.head);

                if (next == Cell::Apple) {
                        // Eating an apple is handled by simply
                        // skipping the step where we erase the
                        // last segment of the snake (the else
                        // branch). We then spawn a new apple.
                        Point apple_location = spawn_apple(&grid);
                        refresh_grid_cell(p->display, color, gd, &grid,
                                          apple_location);
                        (*game_score)++;
                        update_duel_score(p, gd, score_end, *game_score,
                                          is_secondary);
                } else {
                        assert(next == Cell::Empty || next == Cell::Poop);

                        // When no apple is consumed we move
                        // the snake forward by erasing its last
                        // segment. If poop functionality is
                        // enabled we leave it behind.
                        auto tail = snake.body.begin();
                        auto previous = grid[tail->y][tail->x];
                        auto updated =
                            previous == Cell::AppleSnake && config.enable_poop
                                ? Cell::Poop
                                : Cell::Empty;
                        grid[tail->y][tail->x] = updated;
                        update_display_cell(*tail.base());
                        snake.body.erase(tail);
                }

        } else {
                if (grace_used || !config.allow_grace) {
                        *is_game_over = true;
                }

                // We allow the user to change the
                // direction for an additional tick by
                // rolling back the head position.
                Point previous_head = *(snake.body.end() - 1);
                snake.head = {previous_head.x, previous_head.y};

                *grace_used = true;
        }
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

        return new Configuration("Snake Duel", options);
}

SnakeDuelConfiguration *
load_initial_snake_duel_config(PersistentStorage *storage)
{
        int storage_offset = get_settings_storage_offset(Game::SnakeDuel);
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
