#pragma once

#include <stdlib.h>
#include <map>
#include "user_interface_customization.hpp"
#include "configuration.hpp"
#include "../platform/interface/display.hpp"


void setup_display();

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
void render_controls_explanations(const Display &display,
                                  ActionButtonKind button_kind,
                                  std::map<Action, std::string> button_hints);

void render_wrapped_help_text(const Platform &p,
                              const UserInterfaceCustomization &customization,
                              const char *help_text);
void render_wrapped_text(const Platform &p,
                         const UserInterfaceCustomization &customization,
                         const char *text);
std::optional<UserAction> wait_until_green_pressed(const Platform &p);
std::optional<UserAction> wait_until_action_input(const Platform &p,
                                                  Action &action);

ConfigurationDiff *empty_diff();
