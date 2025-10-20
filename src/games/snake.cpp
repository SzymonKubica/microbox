#include "snake.hpp"
#include "2048.hpp"
#include "game_of_life.hpp"
#include "settings.hpp"
#include <optional>
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/constants.hpp"

#define GAME_LOOP_DELAY 100

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
            "Speed", {1, 2, 3, 4}, initial_config->speed);

        auto *accelerate = ConfigurationOption::of_strings(
            "Accelerate", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->accelerate));

        auto *allow_pause = ConfigurationOption::of_strings(
            "Allow pause", {"Yes", "No"},
            map_boolean_to_yes_or_no(initial_config->allow_pause));

        std::vector<ConfigurationOption *> options = {speed, accelerate,
                                                      allow_pause};

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

        ConfigurationOption allow_pause = *config->options[2];
        int curr_allow_pause_idx = allow_pause.currently_selected;
        const char *allow_pause_choice = static_cast<const char **>(
            allow_pause.available_values)[curr_allow_pause_idx];
        game_config->accelerate = extract_yes_or_no_option(allow_pause_choice);
}

Point *spawn_apple(std::vector<std::vector<SnakeGridCell>> *grid);
void re_render_grid_cell(Display *display, GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location);

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
        re_render_grid_cell(p->display, gd, &grid, &snake_head);
        re_render_grid_cell(p->display, gd, &grid, &snake_tail);

        Point *apple_location = spawn_apple(&grid);
        re_render_grid_cell(p->display, gd, &grid, apple_location);

        int evolution_period = (1000 / config.speed) / GAME_LOOP_DELAY;
        int iteration = 0;

        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'spawn' button for too long and
        // the cell flickers instead of getting spawned properly. We implement
        // this using this flag.
        bool action_input_on_last_iteration = false;
        bool is_game_over = false;
        bool is_paused = false;
        while (!is_game_over) {
                Direction dir;
                Action act;
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                        snake.direction = dir;
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

                // Actually move the snake here
                if (!is_paused && iteration == evolution_period - 1) {
                        translate(&snake.head, snake.direction);
                        snake.body->push_back(snake.head);
                        SnakeGridCell previous =
                            grid[snake.head.y][snake.head.x];
                        grid[snake.head.y][snake.head.x] = SnakeGridCell::Snake;
                        re_render_grid_cell(p->display, gd, &grid, &snake.head);

                        if (previous != SnakeGridCell::Apple) {
                                grid[snake.body->begin()->y]
                                    [snake.body->begin()->x] =
                                        SnakeGridCell::Empty;
                                re_render_grid_cell(p->display, gd, &grid,
                                                    snake.body->begin().base());
                                snake.body->erase(snake.body->begin());
                        } else {
                                Point *apple_location = spawn_apple(&grid);
                                re_render_grid_cell(p->display, gd, &grid,
                                                    apple_location);
                        }
                }

                iteration += 1;
                iteration %= evolution_period;
                p->delay_provider->delay_ms(GAME_LOOP_DELAY);
                p->display->refresh();
        }

        p->display->refresh();
        return UserAction::PlayAgain;
}

void re_render_grid_cell(Display *display, GameOfLifeGridDimensions *dimensions,
                         std::vector<std::vector<SnakeGridCell>> *grid,
                         Point *location)
{
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;

        Point grid_loc = *location;
        Point start = {.x = grid_loc.x * width, .y = grid_loc.y * height};

        SnakeGridCell cell_type = (*grid)[grid_loc.y][grid_loc.x];

        switch (cell_type) {
        case SnakeGridCell::Apple:
                display->draw_rectangle(start, width, height, Color::Red, 0,
                                        true);
                break;
        case SnakeGridCell::Snake:
                display->draw_rectangle(start, width, height, Color::Blue, 0,
                                        true);
                break;
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
