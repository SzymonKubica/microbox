#include <cassert>
#include <optional>
#include <string>
#include <stdlib.h>
#include <cstring>

#include "configuration.hpp"
#include "maths_utils.hpp"
#include "constants.hpp"
#include "logging.hpp"
#include "user_interface.hpp"
#include "../platform/interface/controller.hpp"
#include "../common/color.hpp"

#define TAG "configuration"

ConfigurationOption *ConfigurationOption::of_integers(
    const char *name, const std::vector<int> &values, int initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_int_option_values(*option, values);
        option->currently_selected =
            get_config_option_value_index(*option, initial_value);
        return option;
}
ConfigurationOption *
ConfigurationOption::of_strings(const char *name,
                                const std::vector<const char *> &values,
                                const char *initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_string_option_values(*option, values);
        // We need to use this elaborate mechanism of getting the index of the
        // initial value because the config value is also saved in persistent
        // storage without its index. Hence we need to map from the actual value
        // to its index in the options array.
        option->currently_selected =
            get_config_option_string_value_index(*option, initial_value);
        return option;
}

ConfigurationOption *ConfigurationOption::of_colors(
    const char *name, const std::vector<Color> &values, Color initial_value)
{
        ConfigurationOption *option = new ConfigurationOption();
        option->name = name;
        populate_color_option_values(*option, values);
        // We need to use this elaborate mechanism of getting the index of the
        // initial value because the config value is also saved in persistent
        // storage without its index. Hence we need to map from the actual value
        // to its index in the options array.
        option->currently_selected =
            get_config_option_value_index(*option, initial_value);
        return option;
}

void shift_edited_config_option(Configuration &config, ConfigurationDiff &diff,
                                int steps);

/**
 * Modifies the `Configuration` that is passed in. It switches the currently
 * edited option to the one above it (or wraps around to the bottom of the
 * configuration menu). Given that the options are rendered top to bottom it
 * decrements the index of the currently edited config option. The modification
 * that is applied to the config is encoded in the `ConfigurationDiff` object
 * which is then used to partially re-render the menu. */
void switch_edited_config_option_up(Configuration &config,
                                    ConfigurationDiff &diff)
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
void switch_edited_config_option_down(Configuration &config,
                                      ConfigurationDiff &diff)
{
        shift_edited_config_option(config, diff, 1);
}

/**
 * Shifts the currently edited config option. We wrap modulo
 * `config->config_values_len`
 */
void shift_edited_config_option(Configuration &config, ConfigurationDiff &diff,
                                int steps)
{
        LOG_DEBUG(TAG, "Config option index before switching: %d",
                  config.curr_selected_option);
        int config_len = config.options.size();
        diff.previously_edited_option = config.curr_selected_option;
        config.curr_selected_option = mathematical_modulo(
            config.curr_selected_option + steps, config_len);
        diff.currently_edited_option = config.curr_selected_option;
        LOG_DEBUG(TAG, "Config option index after switching: %d",
                  config.curr_selected_option);
}

void shift_current_config_option_value(Configuration &config,
                                       ConfigurationDiff &diff, int steps);

/**
 * Modifies the currently selected configuration bar by incrementing the
 * index of the value of the configuration controlled by this setting.
 */
void increment_current_option_value(Configuration &config,
                                    ConfigurationDiff &diff)
{
        shift_current_config_option_value(config, diff, 1);
}

/**
 * Modifies the currently selected configuration bar by decrementing the
 * index of the value of the configuration controlled by this setting.
 */
void decrement_current_option_value(Configuration &config,
                                    ConfigurationDiff &diff)
{
        shift_current_config_option_value(config, diff, -1);
}

void shift_current_config_option_value(Configuration &config,
                                       ConfigurationDiff &diff, int steps)
{
        assert(config.curr_selected_option != config.options.size());

        int curr_idx = config.curr_selected_option;
        ConfigurationOption *current = config.options[curr_idx];

        current->currently_selected = mathematical_modulo(
            current->currently_selected + steps, current->available_values_len);

        if (config.linked_options.find(curr_idx) !=
            config.linked_options.end()) {
                for (int linked_idx : config.linked_options[curr_idx]) {
                        ConfigurationOption *linked =
                            config.options[linked_idx];
                        linked->currently_selected = mathematical_modulo(
                            linked->currently_selected + steps,
                            linked->available_values_len);
                        diff.modified_options.push_back(linked_idx);
                }
        }

        diff.modified_options.push_back(config.curr_selected_option);
}

void shift_option_value(ConfigurationOption &option, int steps)
{
        option.currently_selected = mathematical_modulo(
            option.currently_selected + steps, option.available_values_len);
}
void increment_option_value(ConfigurationOption &option)
{
        shift_option_value(option, 1);
}
void decrement_option_value(ConfigurationOption &option)
{
        shift_option_value(option, -1);
}

int find_max_config_option_value_text_length(const Configuration &config)
{
        int max_length = 0;
        for (int i = 0; i < config.options.size(); i++) {
                int current_option_value_length;
                ConfigurationOption &current = *config.options[i];
                max_length = std::max(max_length, current.max_config_value_len);
        }
        return max_length;
}

int find_max_config_option_name_text_length(const Configuration &config)
{
        int max_length = 0;
        for (int i = 0; i < config.options.size(); i++) {
                ConfigurationOption &current = *config.options[i];
                max_length = std::max(max_length, (int)strlen(current.name));
        }
        return max_length;
}

int find_max_number_length(const std::vector<int> &numbers);

/**
 * Given a vector of available integer values for a configuration option,
 * it encodes the type information and the maximum number string representation
 * length that is required for rendering in the UI.
 */
void populate_int_option_values(ConfigurationOption &value,
                                const std::vector<int> &available_values)
{
        value.type = ConfigurationOptionType::INT,
        value.available_values_len = available_values.size();
        int *values = new int[value.available_values_len];
        for (int i = 0; i < value.available_values_len; i++) {
                values[i] = available_values[i];
        }
        value.available_values = values;
        value.max_config_value_len = find_max_number_length(available_values);
}

int find_max_string_length(const std::vector<const char *> &strings);
void populate_string_option_values(
    ConfigurationOption &value,
    const std::vector<const char *> &available_values)
{
        value.type = ConfigurationOptionType::STRING,
        value.available_values_len = available_values.size();
        const char **values = new const char *[value.available_values_len];
        for (int i = 0; i < value.available_values_len; i++) {
                values[i] = available_values[i];
        }
        value.available_values = values;
        value.max_config_value_len = find_max_string_length(available_values);
}

int find_max_color_str_length(const std::vector<Color> &available_values);
void populate_color_option_values(ConfigurationOption &value,
                                  const std::vector<Color> &available_values)
{
        value.type = ConfigurationOptionType::COLOR,
        value.available_values_len = available_values.size();
        Color *values = new Color[value.available_values_len];
        for (int i = 0; i < value.available_values_len; i++) {
                values[i] = available_values[i];
        }
        value.available_values = values;
        value.max_config_value_len =
            find_max_color_str_length(available_values);
}

int find_max_number_length(const std::vector<int> &numbers)
{
        int max_len = 0;
        for (int value : numbers) {
                max_len = std::max(
                    max_len, static_cast<int>(std::to_string(value).size()));
        }
        return max_len;
}

int find_max_string_length(const std::vector<const char *> &strings)
{
        int max_len = 0;
        for (const char *value : strings) {
                max_len = std::max(max_len, static_cast<int>(strlen(value)));
        }
        return max_len;
}

int find_max_color_str_length(const std::vector<Color> &colors)
{
        int max_len = 0;
        for (Color value : colors) {
                max_len = std::max(
                    max_len, static_cast<int>(strlen(color_to_string(value))));
        }
        return max_len;
}

int get_config_option_string_value_index(const ConfigurationOption &option,
                                         const char *value)
{
        for (int i = 0; i < option.available_values_len; i++) {
                if (strcmp(
                        static_cast<const char **>(option.available_values)[i],
                        value) == 0) {
                        return i;
                }
        }
        return -1;
}

std::optional<UserAction>
collect_configuration(const Platform &p, Configuration &config,
                      const UserInterfaceCustomization &customization,
                      bool allow_exit, bool should_render_logo)
{
        ConfigurationDiff *diff = new ConfigurationDiff;
        render_config_menu(*p.display, config, *diff, false, customization,
                           should_render_logo);
        if (customization.show_help_text) {
                render_default_controls_explanations(p);
        }
        delete diff;
        while (true) {
                // We get a fresh, empty diff during each iteration to avoid
                // option value text rerendering when they are not modified.
                ConfigurationDiff diff = ConfigurationDiff{};

                // Abstract out the repeatable delay functionality.
                auto move_registered_delay = [&] {
                        p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                };

                auto maybe_action = poll_action_input(p.action_controllers);
                if (maybe_action.has_value()) {
                        Action act = maybe_action.value();
                        /* To make the UI more intuitive, we also allow users to
                        cycle configuration options. This change was inspired by
                        initial play testing by Tomek. */
                        if (act == FORWARD_ACTION) {
                                /*
                                 Note that we re-render before peforming the
                                 move_registered_delay to ensure that the UI is
                                 snappy.
                                 */
                                increment_current_option_value(config, diff);
                                render_config_menu(*p.display, config, diff,
                                                   true, customization,
                                                   should_render_logo);
                                if (!p.display->refresh()) {
                                        return UserAction::CloseWindow;
                                }
                                move_registered_delay();
                                continue;
                        }
                        move_registered_delay();
                        if (act == BACK_ACTION && allow_exit) {
                                return UserAction::Exit;
                        }
                        if (act == HELP_ACTION) {
                                return UserAction::ShowHelp;
                        }
                        if (act == CONFIRM_ACTION) {
                                break;
                        }
                }
                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                Direction dir;
                if (maybe_direction.has_value()) {
                        dir = maybe_direction.value();
                        switch (dir) {
                        case Direction::DOWN:
                                switch_edited_config_option_down(config, diff);
                                break;
                        case Direction::UP:
                                switch_edited_config_option_up(config, diff);
                                break;
                        case Direction::LEFT:
                                decrement_current_option_value(config, diff);
                                break;
                        case Direction::RIGHT:
                                increment_current_option_value(config, diff);
                                break;
                        }

                        render_config_menu(*p.display, config, diff, true,
                                           customization, should_render_logo);
                        if (!p.display->refresh()) {
                                return UserAction::CloseWindow;
                        }

                        p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                }
                if (!p.display->refresh()) {
                        return UserAction::CloseWindow;
                }
                p.time_provider->delay_ms(INPUT_POLLING_DELAY);
        }
        return std::nullopt;
}

/**
 * Responsible for rendering a menu allowing the user to select from a list of
 * available options by 'scrolling' left and right. Each of the list options
 * needs to provide a thumbnail renderer that will be invoked when the game is
 * selected.
 */
std::optional<UserAction> collect_configuration_single_option_with_thumbnails(
    const Platform &p, const UserInterfaceCustomization &customization,
    ConfigurationOption &option, const char *menu_name,
    const std::vector<std::unique_ptr<ThumbnailRenderer>> &thumbnails,
    bool allow_exit, bool should_render_logo)
{
        // Sanity check ensuring that we have enough thumbnail renderers for
        // each available option.
        assert(option.available_values_len == thumbnails.size());

        const auto &display = *p.display;
        display.clear(Black);
        // We insert those dummy configuration options so that the header is
        // positioned
        // high as if two configuration bars were rendered below it. This is a
        // bith hacky but it allows us to reuse the `render_menu_heading`
        // function.
        std::vector<ConfigurationOption *> options = {
            new ConfigurationOption(), new ConfigurationOption()};
        render_menu_heading(display, Configuration(menu_name, options), false,
                            strlen(menu_name), customization,
                            should_render_logo);

        auto render_thumbnail_for_current_selection = [&]() {
                thumbnails[option.currently_selected]->render_thumbnail(
                    p, customization);
        };
        render_thumbnail_for_current_selection();

        // This is needed to avoid re-rendering the wifi icon each time the user
        // scrolls through the configuration options. We still need to call the
        // function in case the wifi status changes while the user is in the
        // configuration menu.
        bool wifi_status_rendered = false;
        if (p.capabilities.has_wifi) {
                update_wifi_status_indicator(p, wifi_status_rendered);
        }

        if (customization.show_help_text) {
                render_default_controls_explanations(p);
        }
        while (true) {
                // Abstract out the repeatable delay functionality.
                auto move_registered_delay = [&] {
                        p.time_provider->delay_ms(MOVE_REGISTERED_DELAY);
                };

                auto maybe_action = poll_action_input(p.action_controllers);
                if (maybe_action.has_value()) {
                        Action act = maybe_action.value();
                        /* To make the UI more intuitive, we also allow users to
                        cycle configuration options. This change was inspired by
                        initial play testing by Tomek. */
                        if (act == FORWARD_ACTION) {
                                /*
                                 Note that we re-render before peforming the
                                 move_registered_delay to ensure that the UI is
                                 snappy.
                                 */
                                increment_option_value(option);
                                render_thumbnail_for_current_selection();
                                if (p.capabilities.has_wifi) {
                                        update_wifi_status_indicator(
                                            p, wifi_status_rendered);
                                }
                                if (!p.display->refresh())
                                        return UserAction::CloseWindow;
                                move_registered_delay();
                                continue;
                        }
                        move_registered_delay();
                        if (act == BACK_ACTION && allow_exit)
                                return UserAction::Exit;
                        if (act == HELP_ACTION)
                                return UserAction::ShowHelp;
                        if (act == CONFIRM_ACTION)
                                break;
                }
                auto maybe_direction =
                    poll_directional_input(p.directional_controllers);
                Direction dir;
                if (maybe_direction.has_value()) {
                        dir = maybe_direction.value();
                        // We only have a single option to configure. Because of
                        // this there is no point to handle Direction::UP/DOWN
                        // inputs.
                        if (dir == Direction::LEFT)
                                decrement_option_value(option);
                        if (dir == Direction::RIGHT)
                                increment_option_value(option);
                        if (dir == Direction::LEFT || dir == Direction::RIGHT) {
                                render_thumbnail_for_current_selection();
                                if (p.capabilities.has_wifi) {
                                        update_wifi_status_indicator(
                                            p, wifi_status_rendered);
                                }
                        }
                        if (!p.display->refresh())
                                return UserAction::CloseWindow;
                        move_registered_delay();
                }
                if (!p.display->refresh())
                        return UserAction::CloseWindow;
                p.time_provider->delay_ms(INPUT_POLLING_DELAY);
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

Configuration::~Configuration()
{
        // We need to free each of the options individually and then free the
        // entire container. This is because the container stores pointers,
        // hence the destructor is not automatically invoked on each of the
        // options pointed to by the pointers.
        for (const auto &option : options) {
                delete option;
        };
        // We don't need to manually free the memory allocated for the vector
        // itself because it's a stack-allocated object and its destructor will
        // be automatically invoked when the Configuration object goes out of
        // scope.
}
ConfigurationOption::~ConfigurationOption()
{
        // We use 'new' to allocate arrays of available values,
        // hence we need to use the corresponding array delete[]
        // to deallocate. Otherwise ASAN complains about it.
        switch (type) {
        case INT:
                delete[] (int *)available_values;
                break;
        case STRING:
                delete[] (char *)available_values;
                break;
        case COLOR:
                delete[] (Color *)available_values;
                break;
        }
}

ConfigurationOption::ConfigurationOption(const ConfigurationOption &other)
{
        type = other.type;
        available_values_len = other.available_values_len;
        currently_selected = other.currently_selected;
        name = other.name;
        max_config_value_len = other.max_config_value_len;

        // Deep copy of the available values array.
        switch (type) {
        case INT: {
                int *values = new int[available_values_len];
                for (int i = 0; i < available_values_len; i++) {
                        values[i] =
                            static_cast<int *>(other.available_values)[i];
                }
                available_values = values;
                break;
        }
        case STRING: {
                const char **values = new const char *[available_values_len];
                for (int i = 0; i < available_values_len; i++) {
                        values[i] = static_cast<const char **>(
                            other.available_values)[i];
                }
                available_values = values;
                break;
        }
        case COLOR: {
                Color *values = new Color[available_values_len];
                for (int i = 0; i < available_values_len; i++) {
                        values[i] =
                            static_cast<Color *>(other.available_values)[i];
                }
                available_values = values;
                break;
        }
        }
}

int ConfigurationOption::get_curr_int_value()
{
        return static_cast<int *>(available_values)[currently_selected];
}

char *ConfigurationOption::get_current_str_value()
{
        return static_cast<char **>(available_values)[currently_selected];
}

Color ConfigurationOption::get_current_color_value()
{
        return static_cast<Color *>(available_values)[currently_selected];
}
