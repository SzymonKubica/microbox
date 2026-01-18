#pragma once

#include "platform/interface/platform.hpp"
#include "user_interface_customization.hpp"
#include "map"
#include <optional>

typedef enum ConfigurationOptionType {
        INT,
        STRING,
        COLOR,
} ConfigurationOptionType;

enum class UserAction {
        PlayAgain,
        Exit,
        ShowHelp,
};

typedef struct ConfigurationOption {
        /**
         * The type of the configurable values. Based on this type we know
         * how to cast the `available_values`.
         */
        ConfigurationOptionType type;
        /**
         * Pointer to the head of the array that stores the finite list of
         * available configuration value options.
         */
        void *available_values;
        /**
         * Number of available configuration options for this
         * `ConfigurationValue`.
         */
        int available_values_len;
        // Currently selected option for this configuration value.
        int currently_selected;
        // Name of the configuration value
        const char *name;
        // Max configuration option string length for UI rendering alignment
        int max_config_value_len;

        ConfigurationOption()
            : type(INT), available_values(nullptr), available_values_len(0),
              currently_selected(0), name(nullptr), max_config_value_len(0)
        {
        }

      public:
        static ConfigurationOption *of_integers(const char *name,
                                                std::vector<int> values,
                                                int initial_value);
        static ConfigurationOption *of_strings(const char *name,
                                               std::vector<const char *> values,
                                               const char *initial_value);
        static ConfigurationOption *of_colors(const char *name,
                                              std::vector<Color> values,
                                              Color initial_value);

        int get_curr_int_value()
        {
                return static_cast<int *>(available_values)[currently_selected];
        }

        char *get_current_str_value()
        {
                return static_cast<char **>(
                    available_values)[currently_selected];
        }

        Color get_current_color_value()
        {
                return static_cast<Color *>(
                    available_values)[currently_selected];
        }

} ConfigurationOption;

/**
 * Given a value that is specifiable by the `ConfigurationOption` it returns the
 * index of this value in the array of available values. This is needed to map
 * from the default game configurations to the default indices when loading the
 * configuration menu.
 */
template <typename T>
int get_config_option_value_index(ConfigurationOption *option, T value)
{
        for (int i = 0; i < option->available_values_len; i++) {
                if (static_cast<T *>(option->available_values)[i] == value) {
                        return i;
                }
        }
        return -1;
}

int get_config_option_string_value_index(ConfigurationOption *option,
                                         const char *value);

struct Configuration;
void populate_options(Configuration *config,
                      std::vector<ConfigurationOption *> options,
                      int currently_selected);

/**
 * A generic container for game configuration values. It allows for storing
 * an arbitrary number of configuration values of type int or string. The idea
 * is that for each configuration value, we provide an array of available
 * values, the currently selected value and the name of configuration option
 * that should be used for rendering in the UI.
 */
struct Configuration {
        /// Name of the configuration group.
        const char *name;
        /**
         * An array of pointers to configuration options available on this
         * configuration object. Each option has a set of values from which the
         * user can select.
         */
        ConfigurationOption **options;
        /// Stores the number of configurable options.
        int options_len;
        /**
         * Represents the configuration value that is currently selected in the
         * UI and is being edited by the user.
         */
        int curr_selected_option;
        /**
         * Allows for 'linking' two configuration option indices. If the user
         * toggles the option with index X and it has an entry in the map, all
         * of the linked indices in the value from the map will be toggled as
         * well. This is useful for allowing users to select related pairs of
         * values, e.g. wifi (ssid, password) pairs.
         */
        std::map<int, std::vector<int>> linked_options;

        Configuration()
            : name(nullptr), options(nullptr), options_len(0),
              curr_selected_option(0), linked_options({})
        {
        }

        Configuration(const char *name,
                      std::vector<ConfigurationOption *> options)
            : name(name), options(nullptr), options_len(options.size()),
              curr_selected_option(0), linked_options({})
        {
                this->options = new ConfigurationOption *[this->options_len];
                populate_options(this, options, 0);
        }

        Configuration(const char *name,
                      std::vector<ConfigurationOption *> options,
                      std::map<int, std::vector<int>> linked_options)
            : name(name), options(nullptr), options_len(options.size()),
              curr_selected_option(0), linked_options(linked_options)
        {
                this->options = new ConfigurationOption *[this->options_len];
                populate_options(this, options, 0);
        }
};

/**
 * Encapsulates the difference in the configuration that has been recorded
 *   after getting user input. This is to allow for selective redrawing of the
 *   configuration menu in the UI and avoid redrawing parts that haven't been
 *   changed. There are two items that can change:
 *   - the currently edited configuration option (the indicator dot needs to be
 *   redrawn)
 *   - the value of a given configuration option changes and needs to be redrawn
 *
 *   For the sake of simplicity, this struct stores fields for both options and
 *   a boolean flag indicating which one has actually changed.
 */
struct ConfigurationDiff {
        /**
         * The two indexes below control tell us what was the previous position
         * of the indicator and the new one. This is used for redrawing the
         * indicator.
         */
        int previously_edited_option;
        int currently_edited_option;
        /**
         * List of indices of options that have changed their values and need
         * to be redrawn
         */
        std::vector<int> modified_options;
};

void switch_edited_config_option_down(Configuration *config,
                                      ConfigurationDiff *diff);
void switch_edited_config_option_up(Configuration *config,
                                    ConfigurationDiff *diff);

void increment_current_option_value(Configuration *config,
                                    ConfigurationDiff *diff);
void decrement_current_option_value(Configuration *config,
                                    ConfigurationDiff *diff);
int find_max_config_option_name_text_length(Configuration *config);
int find_max_config_option_value_text_length(Configuration *config);

/**
 * Given a platform providing the display and controllers implementation
 * and a pointer to the configuration object, this function allows users
 * to modify the configuration. The final settings are written into the
 * `config` struct that is passed as a parameter.
 *
 * It returns true if the configuration was successfully collected. If the user
 * requested to go back, it returns false.
 *
 * While the configuration is being collected, the user has ability to abort the
 * process by either requesting exit or asking for help screen (and possibly
 * more actions in the future). This is controlled by the return parameter. If
 * the return is `std::nullopt`, it means that the configuration was collected
 * and no interrupt action was registered. Otherwise, the function returns
 * some `UserInterruptAction` that needs to be handled by the game loop that
 * started collecting the configuration.
 */
std::optional<UserAction>
collect_configuration(Platform *p, Configuration *config,
                      UserInterfaceCustomization *customization,
                      bool allow_exit = true, bool should_render_logo = false);

void populate_int_option_values(ConfigurationOption *value,
                                std::vector<int> available_values);
void populate_string_option_values(ConfigurationOption *value,
                                   std::vector<const char *> available_values);
void populate_color_option_values(ConfigurationOption *value,
                                  std::vector<Color> available_values);

void populate_options(Configuration *config,
                      std::vector<ConfigurationOption *> options,
                      int currently_selected);

void free_configuration(Configuration *config);

/**
 * Maps from 'Yes', 'No' config option values to boolean.
 */
bool extract_yes_or_no_option(const char *value);
/**
 * Maps boolean to 'Yes', 'No' config option values.
 */
const char *map_boolean_to_yes_or_no(bool value);
