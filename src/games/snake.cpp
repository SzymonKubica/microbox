#include "snake.hpp"
#include "2048.hpp"
#include "game_of_life.hpp"
#include "settings.hpp"
#include <optional>
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"

#define GAME_LOOP_DELAY 50

#define TAG "snake"

SnakeConfiguration DEFAULT_SNAKE_CONFIG = {
    .speed = 2, .accelerate = true, .allow_pause = false};

enum class SnakeGridCell : uint8_t {
        Empty,
        Snake,
        Apple,
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
        SnakeConfiguration config = {
            .speed = 0, .accelerate = false, .allow_pause = 0};

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
        int curr_speeed_idx = speed.currently_selected;
        game_config->speed =
            static_cast<int *>(speed.available_values)[curr_speeed_idx];

        ConfigurationOption accelerate = *config->options[1];
        int curr_accelerate_idx = accelerate.currently_selected;
        const char *accelerate_choice = static_cast<const char **>(
            accelerate.available_values)[curr_accelerate_idx];
        game_config->accelerate = extract_yes_or_no_option(accelerate_choice);

        ConfigurationOption allow_grace = *config->options[2];
        int curr_allow_grace_idx = allow_grace.currently_selected;
        const char *allow_grace_choice = static_cast<const char **>(
            allow_grace.available_values)[curr_allow_grace_idx];
        game_config->allow_grace = extract_yes_or_no_option(allow_grace_choice);

        ConfigurationOption allow_pause = *config->options[3];
        int curr_allow_pause_idx = allow_pause.currently_selected;
        const char *allow_pause_choice = static_cast<const char **>(
            allow_pause.available_values)[curr_allow_pause_idx];
        game_config->allow_pause = extract_yes_or_no_option(allow_pause_choice);
}

Point *spawn_apple(std::vector<std::vector<SnakeGridCell>> *grid);
void re_render_grid_cell(Display *display, GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location);
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

        grid[snake_head.y][snake_head.x] = SnakeGridCell::Snake;
        grid[snake_tail.y][snake_tail.x] = SnakeGridCell::Snake;

        auto update_display_cell = [p, gd, &grid](Point *location) {
                re_render_grid_cell(p->display, gd, &grid, location);
        };
        update_display_cell(&snake_head);
        update_display_cell(&snake_tail);

        Point *apple_location = spawn_apple(&grid);
        update_display_cell(apple_location);

        int evolution_period = (1000 / config.speed) / GAME_LOOP_DELAY;
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
        ;
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
                        switch (act) {
                        case YELLOW:
                                is_paused = !is_paused;
                                break;
                        case RED:
                        case GREEN:
                        case BLUE:
                                break;
                        }
                } else {
                        action_input_on_last_iteration = false;
                }

                // We exract the iteration ending code here as it needs to
                // be called early by the grace handlers to skip snake position
                // update code when doing the grace period.
                auto end_iteration = [p, &iteration, evolution_period]() {
                        iteration += 1;
                        iteration %= evolution_period;
                        p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                        p->display->refresh();
                };

                // Actually move the snake here
                if (!is_paused && iteration == evolution_period - 1) {

                        // This modifies the snake.head pointer.
                        translate(&snake.head, snake.direction);
                        // We check what lies on the cell that the snake has
                        // just entered (todo: bounds check before we do that)
                        if (is_out_of_bounds(&(snake.head), gd)) {
                                if (grace_used || !config.allow_grace) {
                                        is_game_over = true;
                                        break;
                                } else {
                                        // We allow the user to change the
                                        // direction for an additional tick.
                                        snake.head = {snake.body->end()->x,
                                                      snake.body->end()->y};

                                        grace_used = true;

                                        // We short-circuit processing here.
                                        end_iteration();
                                        continue;
                                }
                        }
                        SnakeGridCell next_head =
                            grid[snake.head.y][snake.head.x];

                        if (grace_used && next_head != SnakeGridCell::Snake) {
                                grace_used = false;
                        }

                        switch (next_head) {
                        case SnakeGridCell::Empty: {
                                // Update grid cell at the new location
                                grid[snake.head.y][snake.head.x] =
                                    SnakeGridCell::Snake;
                                snake.body->push_back(snake.head);
                                update_display_cell(&snake.head);

                                // Handling of the tail (remove from snake body
                                // and rerender)
                                auto tail_iter = (*(snake.body)).begin();
                                auto tail = tail_iter.base();
                                grid[tail->y][tail->x] = SnakeGridCell::Empty;
                                update_display_cell(tail);
                                snake.body->erase(tail_iter);

                                break;
                        }
                        case SnakeGridCell::Snake: {
                                if (grace_used || !config.allow_grace) {
                                        is_game_over = true;
                                        break;
                                } else {
                                        // We allow the user to change the
                                        // direction for an additional tick.
                                        snake.head = {snake.body->end()->x,
                                                      snake.body->end()->y};

                                        grace_used = true;

                                        // We short-circuit processing here.
                                        end_iteration();
                                        continue;
                                }
                        }

                        case SnakeGridCell::Apple:
                                // Update grid cell at the new location
                                grid[snake.head.y][snake.head.x] =
                                    SnakeGridCell::Snake;
                                snake.body->push_back(snake.head);
                                update_display_cell(&snake.head);

                                // Eating an apple is handled by simply skipping
                                // the step where we erase the last segment of
                                // the snake. We then spawn a new apple.
                                Point *apple_location = spawn_apple(&grid);
                                re_render_grid_cell(p->display, gd, &grid,
                                                    apple_location);
                                break;
                        }
                }

                end_iteration();
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

void re_render_grid_cell(Display *display, GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location)
{
        int padding = 1;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        Point grid_loc = *location;
        Point start = {.x = left_margin + grid_loc.x * width,
                       .y = top_margin + grid_loc.y * height};

        SnakeGridCell cell_type = (*grid)[grid_loc.y][grid_loc.x];

        switch (cell_type) {
        case SnakeGridCell::Apple: {
                Point apple_center = {.x = start.x + width / 2,
                                      .y = start.y + width / 2};
                display->draw_circle(apple_center, (width - 2 * padding) / 2,
                                     Color::Red, 0, true);
                break;
        }
        case SnakeGridCell::Snake: {
                Point padded_start = {.x = start.x + padding,
                                      .y = start.y + padding};
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, Color::Blue, 0,
                                        true);
                break;
        }
        case SnakeGridCell::Empty:
                display->draw_rectangle(start, width, height, Color::Black, 0,
                                        true);
                break;
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
