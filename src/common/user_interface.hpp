#pragma once

#include <stdlib.h>
#include <map>
#include "user_interface_customization.hpp"
#include "configuration.hpp"
#include "../platform/interface/display.hpp"

void setup_display();

void render_menu_heading(const Display &display, const Configuration &config,
                         bool text_update_only, int text_max_length,
                         const UserInterfaceCustomization &customization,
                         bool should_render_logo = false);
/**
 * Utility function required for thumbnail renderers, the intent is to clear
 * the area where the thumbnail will be rendered and then render a subtitle
 * with the name of the game.
 */
void clear_half_display_and_render_subtitle(
    const Platform &platform, const UserInterfaceCustomization &customization,
    const char *subtitle);
void render_menu_subtitle(const Display &display, const Configuration &config,
                          bool text_update_only, int text_max_length,
                          const UserInterfaceCustomization &customization);
void render_config_menu(const Display &display, const Configuration &config,
                        const ConfigurationDiff &diff, bool text_update_only,
                        const UserInterfaceCustomization &customization,
                        bool should_render_logo = false);

std::optional<UserAction>
collect_string_input(const Platform &p,
                     const UserInterfaceCustomization &customization,
                     const char *input_prompt, char **input);
std::optional<UserAction>
collect_number_input(const Platform &p,
                     const UserInterfaceCustomization &customization,
                     const char *input_prompt, int *input);
void render_logo(const Display &display,
                 const UserInterfaceCustomization &customization,
                 Point position);
void render_default_controls_explanations(const Platform &p);
void update_wifi_status_indicator(const Platform &p, bool &is_rendered);
/**
 * We expose ability to override the Y-axis plancement of the control
 * explanations.
 */
void render_controls_explanations(const Display &display,
                                  ActionButtonKind button_kind,
                                  std::map<Action, std::string> button_hints,
                                  int y_offset_override = 0);

void render_wrapped_help_text(const Platform &p,
                              const UserInterfaceCustomization &customization,
                              const char *help_text);
void render_wrapped_text(const Platform &p,
                         const UserInterfaceCustomization &customization,
                         const char *text);
std::optional<UserAction> wait_until_green_pressed(const Platform &p);
std::optional<UserAction> wait_until_action_input(const Platform &p,
                                                  Action &action);

void render_bar_graph(const Platform &p,
                      const UserInterfaceCustomization &customization,
                      int y_start, const std::vector<std::string> &x_labels,
                      const std::vector<float> &y_labels,
                      std::optional<int> highlighted_bar_index = std::nullopt);

ConfigurationDiff *empty_diff();
