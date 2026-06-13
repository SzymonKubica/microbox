#include <cstring>
#include <cassert>
#include <optional>
#include "snake_common.hpp"
#include "snake.hpp"
#include "../apps/settings.hpp"
#include "../menu.hpp"

#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50

#define TAG "snake"

SnakeConfiguration DEFAULT_CONFIG = {
    .header = {.magic = CONFIGURATION_MAGIC, .version = 1},
    .speed = 7,
    .allow_grace = false,
    .enable_poop = true,
    .allow_pause = false};
using namespace SnakeDefinitions;

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

const char *SnakeGame::get_game_name() const { return "Snake"; };
const char *SnakeGame::get_help_text() const
{
        return "Use the joystick to control where the snake goes."
               "Consume apples to grow the snake. Avoid hitting the walls or "
               "snake's tail. Press 'up' button to (un-)pause.";
};

void update_score(const Platform &p, const SquareCellGridDimensions &dimensions,
                  int score_text_end_location, int score);

UserAction SnakeGame::app_loop(const Platform &p,
                               const UserInterfaceCustomization &customization,
                               const SnakeConfiguration &config) const
{
        LOG_DEBUG(TAG, "Entering Snake game loop");

        int game_cell_width = DEFAULT_SNAKE_GAME_CELL_WIDTH;
        auto gd =
            std::unique_ptr<SquareCellGridDimensions>(calculate_grid_dimensions(
                p.display->get_width(), p.display->get_height(),
                p.display->get_display_corner_radius(), game_cell_width));
        int rows = gd->rows;
        int cols = gd->cols;

        LOG_DEBUG(TAG, "Rendering snake game area.");
        draw_grid_frame(p, customization, *gd);

        std::vector<std::vector<Cell>> grid(rows, std::vector<Cell>(cols));

        // We render the 'Score:' text only once but including the empty space
        // required for the score. This is needed to ensure that the score
        // section above the grid is properly centered. When rendering the
        // actual score count, we need to cancel out the three spaces and
        // subtract them from score_end pixel position. The function above
        // returns the ending position of the score text so that we can render
        // the score appropriately.
        int score_text_end_x =
            render_centered_above_frame(p, *gd, "Score:    ");

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
        auto render_score = [p, &gd, &grid, score_text_end_x](int score) {
                update_score(p, *gd, score_text_end_x, score);
        };
        // After the value of a given cell in the grid is changed, this
        // re-renders that single cell in the display.
        auto render_cell = [p, &gd, &grid, customization](Point &location) {
                refresh_grid_cell(*p.display, customization.accent_color,
                                  *gd.get(), grid, location);
        };
        // Renders the snake's head including the neck (2nd segment right behind
        // the head).
        auto render_head = [p, &gd, &grid, customization](Snake &snake) {
                auto neck = snake.get_neck();
                render_segment_connection(*p.display,
                                          customization.accent_color, *gd.get(),
                                          neck, snake.head);
                render_snake_head(*p.display, customization.accent_color,
                                  *gd.get(), snake);
        };

        // Initial score rendering to complete the game grid first, before the
        // snake gets rendered.
        render_score(0);
        if (!p.display->refresh()) {
                return UserAction::CloseWindow;
        }

        // Initialize game entities
        // The snake starts in the middle of the area pointing to the right.
        Snake snake{{.x = cols / 2, .y = rows / 2}, Direction::RIGHT};
        set_cell(snake.head, Cell::Snake);
        set_cell(snake.tail, Cell::Snake);
        Point apple_location = spawn_apple(grid);

        // Render initial state of the game entities.
        render_cell(snake.tail);
        render_head(snake);
        render_cell(apple_location);

        GameLoopState state{config.speed};

        // Convenience funtion to ensure each short-circuit exit of the
        // loop iteration actually increments the counter and waits a bit.
        auto increment_iteration_and_wait =
            [p, &state]() -> std::optional<UserAction> {
                state.increment_iteration();
                p.time_provider->delay_ms(GAME_LOOP_DELAY);
                if (!p.display->refresh()) {
                        return UserAction::CloseWindow;
                }
                return std::nullopt;
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

                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                if (maybe_direction.has_value() &&
                    !is_opposite(maybe_direction.value(), snake.direction)) {
                        // We prevent instant game-over when user presses the
                        // direction that is opposite to the current direction
                        // of the snake.
                        chosen_snake_direction = maybe_direction.value();
                }

                Action act;
                auto maybe_action = poll_action_input(p.action_controllers);
                bool action_taken = maybe_action.has_value();

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
                        if (increment_iteration_and_wait().has_value()) {
                                return UserAction::CloseWindow;
                        };
                        continue;
                }

                Cell next;
                snake.direction = chosen_snake_direction;
                snake.take_step();

                // Check for failure conditions.
                bool wall_hit = is_out_of_bounds(snake.head, *gd);
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
                        if (increment_iteration_and_wait().has_value()) {
                                return UserAction::CloseWindow;
                        }
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
                        Point apple_loc = spawn_apple(grid);
                        game_score++;
                        render_cell(apple_loc);
                        render_score(game_score);
                        if (increment_iteration_and_wait().has_value()) {
                                return UserAction::CloseWindow;
                        }
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
                if (increment_iteration_and_wait().has_value()) {
                        return UserAction::CloseWindow;
                }
        }

        if (!p.display->refresh()) {
                return UserAction::CloseWindow;
        } else {
                return UserAction::PauseAndPlayAgain;
        }
}

/**
 * Re-renders the text location above the grid informing the user about the
 * current score in the game.
 */
void update_score(const Platform &p, const SquareCellGridDimensions &dimensions,
                  int score_text_end_location, int score)
{

        auto [fw, fh] = p.display->get_font_configuration().font_dimensions;
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
        int start_position = score_text_end_location - 3 * fw;
        render_text_above_frame_starting_from(p, dimensions, buffer,
                                              start_position, true);
}

/**
 * Forward declarations of functions related to configuration manipulation.
 */
SnakeConfiguration *load_initial_snake_config(const PersistentStorage &storage);
Configuration *assemble_snake_configuration(const PersistentStorage &storage);
void extract_game_config(SnakeConfiguration &game_config,
                         const Configuration &config);

std::optional<UserAction>
SnakeGame::collect_config(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          SnakeConfiguration &game_config) const
{
        auto config = std::unique_ptr<Configuration>(
            assemble_snake_configuration(*p.persistent_storage));

        auto interrupt = collect_configuration(p, *config, customization);
        if (interrupt)
                return interrupt;

        extract_game_config(game_config, *config.get());
        return std::nullopt;
}

Configuration *assemble_snake_configuration(const PersistentStorage &storage)
{
        auto initial_config = std::unique_ptr<SnakeConfiguration>(
            load_initial_snake_config(storage));

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

SnakeConfiguration *load_initial_snake_config(const PersistentStorage &storage)
{
        int storage_offset = get_settings_storage_offset(Game::Snake);
        LOG_DEBUG(TAG, "Loading config from offset %d", storage_offset);

        SnakeConfiguration config;
        LOG_DEBUG(TAG, "Trying to load settings from the persistent storage");
        storage.get(storage_offset, config);

        SnakeConfiguration *output = new SnakeConfiguration();
        if (!config.header.validate_against(DEFAULT_CONFIG)) {
                LOG_DEBUG(TAG, "The storage does not contain a valid "
                               "snake configuration, using default values.");
                memcpy(output, &DEFAULT_CONFIG, sizeof(SnakeConfiguration));
                storage.put(storage_offset, DEFAULT_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(SnakeConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded snake configuration: speed=%d, enable_poop=%d, "
                  "allow_grace=%d, "
                  "allow_pause=%d",
                  output->speed, output->enable_poop, output->allow_grace,
                  output->allow_pause);

        return output;
}

void extract_game_config(SnakeConfiguration &game_config,
                         const Configuration &config)
{
        ConfigurationOption speed = *config.options[0];
        ConfigurationOption enable_poop = *config.options[1];
        ConfigurationOption allow_grace = *config.options[2];
        ConfigurationOption allow_pause = *config.options[3];

        auto yes_or_no_option_to_bool = [](ConfigurationOption option) {
                return extract_yes_or_no_option(option.get_current_str_value());
        };

        game_config.speed = speed.get_curr_int_value();
        game_config.enable_poop = yes_or_no_option_to_bool(enable_poop);
        game_config.allow_grace = yes_or_no_option_to_bool(allow_grace);
        game_config.allow_pause = yes_or_no_option_to_bool(allow_pause);
}

void SnakeGame::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{

        const Display &display = *platform.display;

        clear_half_display_and_render_subtitle(platform, customization,
                                               "Snake");

        int game_cell_width = DEFAULT_SNAKE_GAME_CELL_WIDTH;
        auto gd =
            std::unique_ptr<SquareCellGridDimensions>(calculate_grid_dimensions(
                platform.display->get_width(), platform.display->get_height(),
                platform.display->get_display_corner_radius(),
                game_cell_width));

        // After the value of a given cell in the grid is changed, this
        // re-renders that single cell in the display.
        auto render_cell = [platform, &gd, customization](Point location,
                                                          Cell type) {
                render_grid_cell(*platform.display, customization.accent_color,
                                 *gd.get(), type, location);
        };
        // Renders the snake's head including the neck (2nd segment right behind
        // the head).
        auto render_head = [platform, &gd, customization](Snake &snake) {
                auto neck = snake.get_neck();
                render_segment_connection(*platform.display,
                                          customization.accent_color, *gd.get(),
                                          neck, snake.head);
                render_snake_head(*platform.display, customization.accent_color,
                                  *gd.get(), snake);
        };

        auto render_connection = [platform, &gd, customization](Point first,
                                                                Point second) {
                render_segment_connection(*platform.display,
                                          customization.accent_color, *gd.get(),
                                          first, second);
        };

        int rows = gd->rows;
        int cols = gd->cols;
        Snake snake{{.x = cols / 2, .y = rows / 2}, Direction::RIGHT};

        render_head(snake);
        render_cell(snake.get_neck(), Cell::Snake);

        // Here we control the layout of the snake's tail by specifying
        // which direction it went starting from the head and going back.
        Point trail_part = {0, -1};
        std::vector<Point> snake_trail;
        snake_trail.push_back(trail_part);
        auto add_tail_segment = [&](Direction direction) {
                translate(trail_part, direction);
                snake_trail.push_back(trail_part);
        };
        add_tail_segment(Direction::LEFT);
        add_tail_segment(Direction::DOWN);
        add_tail_segment(Direction::DOWN);
        add_tail_segment(Direction::DOWN);
        add_tail_segment(Direction::DOWN);
        add_tail_segment(Direction::RIGHT);
        add_tail_segment(Direction::RIGHT);
        add_tail_segment(Direction::RIGHT);
        add_tail_segment(Direction::RIGHT);
        add_tail_segment(Direction::UP);

        Point last = snake.get_neck();
        for (const auto &p : snake_trail) {
                Point translated = snake.get_neck() + p;
                render_connection(last, translated);
                render_cell(translated, Cell::Snake);
                last = translated;
        }

        Point apple = snake.head;
        translate(apple, Direction::RIGHT);
        translate(apple, Direction::RIGHT);
        translate(apple, Direction::UP);
        render_cell(apple, Cell::Apple);
}
