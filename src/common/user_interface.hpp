#pragma once

#include <stdlib.h>
#include "user_interface_customization.hpp"
#include "configuration.hpp"
#include "platform/interface/display.hpp"

void setup_display();

void render_config_menu(Display *display, Configuration *config,
                        ConfigurationDiff *diff, bool text_update_only,
                        UserInterfaceCustomization *customization, bool should_render_logo = false);

char* collect_string_input(Platform *p,
                          UserInterfaceCustomization *customization);
void render_logo(Display *display, UserInterfaceCustomization *customization,
                 Point position);
void render_controls_explanations(Display *display);

void render_wrapped_help_text(Platform *p,
                              UserInterfaceCustomization *customization,
                              const char *help_text);
void render_wrapped_text(Platform *p,
                              UserInterfaceCustomization *customization,
                              const char *text);
void wait_until_green_pressed(Platform *p);

ConfigurationDiff *empty_diff();
