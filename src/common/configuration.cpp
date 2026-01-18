#include <cassert>
#include <optional>
#include <string>
#include <stdlib.h>

#include "configuration.hpp"
#include "maths_utils.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "user_interface.hpp"
#include "platform/interface/controller.hpp"
#include "platform/interface/color.hpp"

#define TAG "configuration"

ConfigurationOption *ConfigurationOption::of_integers(const char *name,
                                                      std::vector<int> values,
                                                      int initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_int_option_values(option, values);
        option->currently_selected =
            get_config_option_value_index(option, initial_value);
        return option;
}
ConfigurationOption *
ConfigurationOption::of_strings(const char *name,
                                std::vector<const char *> values,
                                const char *initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_string_option_values(option, values);
        // We need to use this elaborate mechanism of getting the index of the
        // initial value because the config value is also saved in persistent
        // storage without its index. Hence we need to map from the actual value
        // to its index in the options array.
        option->currently_selected =
            get_config_option_string_value_index(option, initial_value);
        return option;
}

ConfigurationOption *ConfigurationOption::of_colors(const char *name,
                                                    std::vector<Color> values,
                                                    Color initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_color_option_values(option, values);
        // We need to use this elaborate mechanism of getting the index of the
        // initial value because the config value is also saved in persistent
        // storage without its index. Hence we need to map from the actual value
        // to its index in the options array.
        option->currently_selected =
            get_config_option_value_index(option, initial_value);
        return option;
}

void shift_edited_config_option(Configuration *config, ConfigurationDiff *diff,
                                int steps);

/**
 * Modifies the `Configuration` that is passed in. It switches the currently
 * edited option to the one above it (or wraps around to the bottom of the
 * configuration menu). Given that the options are rendered top to bottom it
 * decrements the index of the currently edited config option. The modification
 * that is applied to the config is encoded in the `ConfigurationDiff` object
 * which is then used to partially re-render the menu. */
void switch_edited_config_option_up(Configuration *config,
                                    ConfigurationDiff *diff)
{
        shift_edited_config_option(config, diff, -1);
}

/**
 * Modifies the `Configuration` that is passed in. It switches the currently
 * edited option to the one below it (or wraps around to the top of the
 * configuration menu). Given that the options are rendered top to bottom it
 * increments the index of the currently edited config option. The modification
 * that is applied to the config is encoded in the `ConfigurationDiff` object
 * which is then used to partially re-render the menu. */
void switch_edited_config_option_down(Configuration *config,
                                      ConfigurationDiff *diff)
{
        shift_edited_config_option(config, diff, 1);
}

/**
 * Shifts the currently edited config option. We wrap modulo
 * `config->config_values_len`
 */
void shift_edited_config_option(Configuration *config, ConfigurationDiff *diff,
                                int steps)
{
        LOG_DEBUG(TAG, "Config option index before switching: %d",
                  config->curr_selected_option);
        int config_len = config->options_len;
        diff->previously_edited_option = config->curr_selected_option;
        config->curr_selected_option = mathematical_modulo(
            config->curr_selected_option + steps, config_len);
        diff->currently_edited_option = config->curr_selected_option;
        LOG_DEBUG(TAG, "Config option index after switching: %d",
                  config->curr_selected_option);
}

void shift_current_config_option_value(Configuration *config,
                                       ConfigurationDiff *diff, int steps);

/**
 * Modifies the currently selected configuration bar by incrementing the
 * index of the value of the configuration controlled by this setting.
 */
void increment_current_option_value(Configuration *config,
                                    ConfigurationDiff *diff)
{
        shift_current_config_option_value(config, diff, 1);
}

/**
 * Modifies the currently selected configuration bar by decrementing the
 * index of the value of the configuration controlled by this setting.
 */
void decrement_current_option_value(Configuration *config,
                                    ConfigurationDiff *diff)
{
        shift_current_config_option_value(config, diff, -1);
}

void shift_current_config_option_value(Configuration *config,
                                       ConfigurationDiff *diff, int steps)
{
        assert(config->curr_selected_option != config->options_len);

        int curr_idx = config->curr_selected_option;
        ConfigurationOption *current = config->options[curr_idx];

        current->currently_selected = mathematical_modulo(
            current->currently_selected + steps, current->available_values_len);

        if (config->linked_options.contains(curr_idx)) {
                for (int linked_idx : config->linked_options[curr_idx]) {
                        ConfigurationOption *linked =
                            config->options[linked_idx];
                        linked->currently_selected = mathematical_modulo(
                            linked->currently_selected + steps,
                            linked->available_values_len);
                        diff->modified_options.push_back(linked_idx);
                }
        }

        diff->modified_options.push_back(config->curr_selected_option);
}

/** For some reason when compiling the `max` function is not available.
 */
int max(int a, int b) { return (a > b) ? a : b; }

int find_max_config_option_value_text_length(Configuration *config)
{
        int max_length = 0;
        for (int i = 0; i < config->options_len; i++) {
                int current_option_value_length;
                ConfigurationOption *current = config->options[i];
                max_length = max(max_length, current->max_config_value_len);
        }
        return max_length;
}

int find_max_config_option_name_text_length(Configuration *config)
{
        int max_length = 0;
        for (int i = 0; i < config->options_len; i++) {
                ConfigurationOption *current = config->options[i];
                max_length = max(max_length, strlen(current->name));
        }
        return max_length;
}

int find_max_number_length(std::vector<int> numbers);

/**
 * Given a vector of available integer values for a configuration option,
 * it encodes the type information and the maximum number string representation
 * length that is required for rendering in the UI.
 */
void populate_int_option_values(ConfigurationOption *value,
                                std::vector<int> available_values)
{
        value->type = ConfigurationOptionType::INT,
        value->available_values_len = available_values.size();
        int *values = new int[value->available_values_len];
        for (int i = 0; i < value->available_values_len; i++) {
                values[i] = available_values[i];
        }
        value->available_values = values;
        value->max_config_value_len = find_max_number_length(available_values);
}

int find_max_string_length(std::vector<const char *> strings);
void populate_string_option_values(ConfigurationOption *value,
                                   std::vector<const char *> available_values)
{
        value->type = ConfigurationOptionType::STRING,
        value->available_values_len = available_values.size();
        const char **values = new const char *[value->available_values_len];
        for (int i = 0; i < value->available_values_len; i++) {
                values[i] = available_values[i];
        }
        value->available_values = values;
        value->max_config_value_len = find_max_string_length(available_values);
}

int find_max_color_str_length(std::vector<Color> available_values);
void populate_color_option_values(ConfigurationOption *value,
                                  std::vector<Color> available_values)
{
        value->type = ConfigurationOptionType::COLOR,
        value->available_values_len = available_values.size();
        Color *values = new Color[value->available_values_len];
        for (int i = 0; i < value->available_values_len; i++) {
                values[i] = available_values[i];
        }
        value->available_values = values;
        value->max_config_value_len =
            find_max_color_str_length(available_values);
}

void populate_options(Configuration *config,
                      std::vector<ConfigurationOption *> options,
                      int currently_selected)
{
        config->options_len = options.size();
        config->options = new ConfigurationOption *[config->options_len];

        for (int i = 0; i < options.size(); i++) {
                config->options[i] = options[i];
        }
        config->curr_selected_option = currently_selected;
}

int find_max_number_length(std::vector<int> numbers)
{
        int max_len = 0;
        for (int value : numbers) {
                max_len = max(max_len, std::to_string(value).size());
        }
        return max_len;
}

int find_max_string_length(std::vector<const char *> strings)
{
        int max_len = 0;
        for (const char *value : strings) {
                max_len = max(max_len, strlen(value));
        }
        return max_len;
}

int find_max_color_str_length(std::vector<Color> colors)
{
        int max_len = 0;
        for (Color value : colors) {
                max_len = max(max_len, strlen(color_to_string(value)));
        }
        return max_len;
}

void free_configuration(Configuration *config)
{
        for (int i = 0; i < config->options_len; i++) {
                ConfigurationOption *option = config->options[i];
                free(option->available_values);
                delete config->options[i];
        }
        delete config;
}

int get_config_option_string_value_index(ConfigurationOption *option,
                                         const char *value)
{
        for (int i = 0; i < option->available_values_len; i++) {
                if (strcmp(
                        static_cast<const char **>(option->available_values)[i],
                        value) == 0) {
                        return i;
                }
        }
        return -1;
}

std::optional<UserAction>
collect_configuration(Platform *p, Configuration *config,
                      UserInterfaceCustomization *customization,
                      bool allow_exit, bool should_render_logo)
{
        ConfigurationDiff *diff = empty_diff();
        render_config_menu(p->display, config, diff, false, customization,
                           should_render_logo);
        if (customization->show_help_text) {
                render_controls_explanations(p->display);
        }
        free(diff);
        while (true) {
                Action act;
                Direction dir;
                // We get a fresh, empty diff during each iteration to avoid
                // option value text rerendering when they are not modified.
                ConfigurationDiff *diff = empty_diff();
                bool confirmation_bar_selected =
                    config->curr_selected_option == config->options_len;

                // Abstract out the repeatable delay functionality.
                auto move_registered_delay = [&] {
                        p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
                };

                if (action_input_registered(p->action_controllers, &act)) {
                        /* To make the UI more intuitive, we also allow users to
                        cycle configuration options. This change was inspired by
                        initial play testing by Tomek. */
                        if (act == Action::GREEN) {
                                /*
                                 Note that we re-render before peforming the
                                 move_registered_delay to ensure that the UI is
                                 snappy.
                                 */
                                increment_current_option_value(config, diff);
                                render_config_menu(p->display, config, diff,
                                                   true, customization,
                                                   should_render_logo);
                                move_registered_delay();
                                free(diff);
                                continue;
                        }
                        move_registered_delay();
                        if (act == Action::BLUE && allow_exit) {
                                return UserAction::Exit;
                        }
                        if (act == Action::YELLOW) {
                                return UserAction::ShowHelp;
                        }
                        if (act == Action::RED) {
                                break;
                        }
                }
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                        switch (dir) {
                        case DOWN:
                                switch_edited_config_option_down(config, diff);
                                break;
                        case UP:
                                switch_edited_config_option_up(config, diff);
                                break;
                        case LEFT:
                                decrement_current_option_value(config, diff);
                                break;
                        case RIGHT:
                                increment_current_option_value(config, diff);
                                break;
                        }

                        render_config_menu(p->display, config, diff, true,
                                           customization, should_render_logo);

                        p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
                }
                p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
                p->display->refresh();
                free(diff);
        }
        return std::nullopt;
}

bool extract_yes_or_no_option(const char *value)
{
        if (strlen(value) == 3 && strncmp(value, "Yes", 3) == 0) {
                return true;
        }
        return false;
}

const char *map_boolean_to_yes_or_no(bool value)
{
        return value ? "Yes" : "No";
}
