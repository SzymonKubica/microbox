#pragma once

#include <stdlib.h>
#include <map>
#include <variant>
#include "user_interface_customization.hpp"
#include "configuration.hpp"
#include "platform/interface/display.hpp"

void setup_display();

void render_config_menu(Display *display, Configuration *config,
                        ConfigurationDiff *diff, bool text_update_only,
                        UserInterfaceCustomization *customization,
                        bool should_render_logo = false);

std::variant<char *, UserAction>
collect_string_input(Platform *p, UserInterfaceCustomization *customization,
                     const char *input_prompt);
void render_logo(Display *display, UserInterfaceCustomization *customization,
                 Point position);
void render_controls_explanations(Display *display);
void render_controls_explanations(Display *display,
                                  std::map<Action, std::string> button_hints);

void render_wrapped_help_text(Platform *p,
                              UserInterfaceCustomization *customization,
                              const char *help_text);
void render_wrapped_text(Platform *p, UserInterfaceCustomization *customization,
                         const char *text);
std::optional<UserAction> wait_until_green_pressed(Platform *p);
std::variant<Action, UserAction> wait_until_action_input(Platform *p);

ConfigurationDiff *empty_diff();
