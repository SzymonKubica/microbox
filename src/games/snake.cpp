#include "snake.hpp"
#include "settings.hpp"
#include <optional>
#include "game_menu.hpp"

#include "../common/configuration.hpp"
#include "../common/logging.hpp"
#include "../common/constants.hpp"

#define TAG "snake"

SnakeConfiguration DEFAULT_SNAKE_CONFIG = {
    .speed = 25, .accelerate = true, .allow_pause = false};

UserAction snake_loop(Platform *p,
                            UserInterfaceCustomization *customization);

void Snake::game_loop(Platform *p, UserInterfaceCustomization *customization) {
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
                        LOG_DEBUG(TAG,
                                  "User requested snake help screen");
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
            "Speed", {10, 15, 25, 30, 35}, initial_config->speed);

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

        ConfigurationOption accelerate = *config->options[0];
        int curr_accelerate_idx = accelerate.currently_selected;
        const char *accelerate_choice = static_cast<const char **>(
            accelerate.available_values)[curr_accelerate_idx];
        game_config->accelerate = extract_yes_or_no_option(accelerate_choice);

        ConfigurationOption allow_pause = *config->options[0];
        int curr_allow_pause_idx = allow_pause.currently_selected;
        const char *allow_pause_choice = static_cast<const char **>(
            allow_pause.available_values)[curr_allow_pause_idx];
        game_config->accelerate = extract_yes_or_no_option(allow_pause_choice);
}

UserAction snake_loop(Platform *p,
                            UserInterfaceCustomization *customization)
{
        LOG_DEBUG(TAG, "Entering Snake game loop");
        SnakeConfiguration config;

        auto maybe_interrupt =
            collect_snake_config(p, &config, customization);

        if (maybe_interrupt) {
                return maybe_interrupt.value();
        }

        //MinesweeperGridDimensions *gd = calculate_grid_dimensions(
        //    p->display->get_width(), p->display->get_height(),
        //    p->display->get_display_corner_radius());
        //int rows = gd->rows;
        //int cols = gd->cols;

        //draw_game_canvas(p, gd, customization);
        //LOG_DEBUG(TAG, "Minesweeper game canvas drawn.");

        //p->display->refresh();

        //std::vector<std::vector<MinesweeperGridCell>> grid(
        //    rows, std::vector<MinesweeperGridCell>(cols));

        ///* We only place bombs after the user selects the cell to uncover.
        //   This avoids situations where the first selected cell is a bomb
        //   and the game is immediately over without user's logical error. */
        //bool bombs_placed = false;

        //Point caret_position = {.x = 0, .y = 0};
        //draw_caret(p->display, &caret_position, gd);
        //LOG_DEBUG(TAG, "Caret rendered at initial position.");

        int total_uncovered = 0;

        // To avoid button debounce issues, we only process action input if
        // it wasn't processed on the last iteration. This is to avoid
        // situations where the user holds the 'spawn' button for too long and
        // the cell flickers instead of getting spawned properly. We implement
        // this using this flag.
        bool action_input_on_last_iteration = false;
        bool is_game_over = false;
        while (!is_game_over ) {
                Direction dir;
                Action act;
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                }
                if (action_input_registered(p->action_controllers, &act) &&
                    !action_input_on_last_iteration) {
                } else {
                        action_input_on_last_iteration = false;
                }
                p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
        }

        p->display->refresh();
        return UserAction::PlayAgain;
}
