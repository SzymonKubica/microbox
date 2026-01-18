#include "user_interface.hpp"
#include "configuration.hpp"
#include "platform/interface/color.hpp"
#include "platform/interface/display.hpp"
#include "../common/logging.hpp"
#include "constants.hpp"
#include "point.hpp"
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cstring>

#define GRID_BG_COLOR White
#define TAG "user_interface"

#define SELECTOR_CIRCLE_RADIUS 5

// Maximum length of config option value text in characters.
// This is needed to ensure that the config bars don't overflow the display.
#define MAX_CONFIG_OPTION_VALUE_LENGTH 13

/* User Interface */

/* Helper functions used by draw_configuration_menu */

inline int get_centering_margin(int screen_width, int text_length,
                                int font_width);
void render_config_bar_centered(Display *display, int y_start,
                                int option_text_max_len, int value_text_max_len,
                                const char *option_text, const char *value_text,
                                bool is_already_rendered,
                                bool update_value_cell,
                                UserInterfaceCustomization *customization);
void render_text_bar_centered(Display *display, int y_start,
                              int option_text_max_len, int value_text_max_len,
                              const char *text, bool is_already_rendered,
                              UserInterfaceRenderingMode rendering_mode,
                              Color background_color = Black,
                              Color text_color = White,
                              int font_width = FONT_WIDTH,
                              FontSize font_size = Size16);
void render_circle_selector(Display *display, bool already_rendered, int x_axis,
                            int *y_positions, int y_positions_len,
                            int prev_pos_idx, int curr_pos_idx, int radius,
                            Color bg_color = Black,
                            Color circle_color = DarkBlue);
int calculate_section_spacing(int display_height, int config_bar_num,
                              int bar_height, int gap_between_bars_height,
                              FontSize heading_font_size);
int *calculate_config_bar_positions(int y_spacing, FontSize heading_font_size,
                                    int bar_height, int bar_gap_height,
                                    int config_bar_num);

/**
 * Allows for rendering a centered configuration bar. A config bar consists of
 * two parts: the option text and the value cell. The option text displays the
 * name of the config option whereas the value cell shows the currently selected
 * value. It is supposed to render as follows:
 *   _____________________________________________
 *  /                     _______________________ \
 * |     Option text     |       Value cell      | |
 *  \                     ----------------------- /
 *   ---------------------------------------------
 * So the configuration bar is a large rounded rect that has the smaller rounded
 * rect for the actual value. It is very important that the background of the
 * value cell is white and the text is black. This is required by the specifics
 * of the display that we are using for the console. Given that it only allows
 * for redrawing one pixel at a time, and the speed of redrawing white/black is
 * the highest (less information required to transfer over the wire), we need
 * to opt for black-on-white text for parts of the game that get redrawn
 * frequently.
 *
 * The function takes the following parameters:
 * @param `display` A pointer to the implementation of the display to render on.
 * @param `y_start` The y coordinate of the top left corner of the config bar.
 * This function only enforces horizontal centering, the caller is responsible
 *        for enforcing proper vertical spacing using this argument.
 * @param `option_text_max_len` The maximum length of the option text, required
 * for centering. The idea here is that if you want to render multiple
 * configuration bars, you need to find the max across their option names and
 * pass it in when rendering each bar. This will ensure that all config bars are
 *        of the same width and are properly centered.
 * @param `value_text_max_len` The max length of the value text in the value
 * cell, similar to the `option_text_max_len` it is used for proper centering
 * and consistent spacing across multiple config bars rendered on the same UI.
 * @param `option_text` The text for the name of the config option to display on
 *        the config bar.
 * @param `is_already_rendered` Because of the limited speed of the display of
 *        the console, we need to be really efficient with what we render. If
 *        we want the config menu to be snappy, there is no point in redrawing
 *        the entire bar if the value cell changes. Because of this the caller
 *        needs to specify this flag to inform the function whether it is
 * supposed to render everything or just redraw the value cell.
 * @param `update_value_cell` If set, the function will only redraw the value
 * cell, the entire config bar will not be rerendered
 * @param `customzation` Controls the look and feel of the UI. If customization
 * specifies that we should be using the Minimalistic rendering mode, this
 * will use regular rectangles with no fill instead of the default filled
 * rounded rectangles.
 */
void render_config_bar_centered(Display *display, int y_start,
                                int option_text_max_len, int value_text_max_len,
                                const char *option_text, const char *value_text,
                                bool is_already_rendered,
                                bool update_value_cell,
                                UserInterfaceCustomization *customization)
{

        // For all selector buttons we need to find the one that has the longest
        // text and then put two spaces between the text of that one and the
        // selector option blob (the thing that displays the actual value of
        // the selected option).
        int option_value_gap_len = 2;
        // To center the text properly we need to get the maximum length in
        // characters of the text that will be displayed on the configuration
        // bars. The configuraion bars are of the form:
        // <option_name_text>__<value_text> with '__' denoting two spaces
        // between the option text and the value text.
        int text_len =
            option_text_max_len + option_value_gap_len + value_text_max_len;

        int h = display->get_height();
        int w = display->get_width();
        int fw = FONT_WIDTH;
        int fh = FONT_SIZE;

        int left_margin = get_centering_margin(w, fw, text_len);

        // We determine centering of the configuration bars based on the
        // position of the text. The actual rounded rect that contains the text
        // is slightly larger. Because of this we add 'padding' to the config
        // bar by making it start slightly to the left and up from the text. We
        // then make it longer and taller to properly contain all of the text.
        int h_padding = fw / 2;
        int v_padding = fh / 2;
        Point bar_start = {.x = left_margin - h_padding,
                           .y = y_start - v_padding};

        int bar_width = text_len * fw + 2 * h_padding;
        // The value cell is the small rounded rect inside of the config bar
        // that contains the actual value that the config bar is modifying.
        // Its rounded rect starts in the middle of the empty space between the
        // option text and the value text, hence we are dividing the separator
        // by two.
        int value_cell_x =
            left_margin + (option_text_max_len + option_value_gap_len / 2) * fw;
        // The value cell has to be smaller than the containing config bar,
        // because of this we add negative padding to make it fit.
        int value_cell_v_padding = fh / 4;
        int value_cell_y = y_start - value_cell_v_padding;
        Point value_cell_start = {.x = value_cell_x, .y = value_cell_y};

        Color accent_color = customization->accent_color;

        if (!is_already_rendered) {
                Point bar_name_str_start = {.x = left_margin, .y = y_start};

                if (customization->rendering_mode == Detailed) {
                        // Draw the background for the two configuration cells.
                        display->draw_rounded_rectangle(
                            bar_start, bar_width, fh * 2, fh, accent_color);
                        // Draw the actual name of the config bar.
                        display->draw_string(
                            bar_name_str_start, (char *)option_text, Size16,
                            accent_color,
                            get_good_contrast_text_color(accent_color));
                } else {
                        // The only other option supported right now is the
                        // `Minimalistic` rendering mode, we render it below
                        display->draw_rectangle(bar_start, bar_width, fh * 2,
                                                accent_color, 1, false);
                        // Draw the actual name of the config bar.
                        display->draw_string(bar_name_str_start,
                                             (char *)option_text, Size16, Black,
                                             White);
                }
        }

        // Draw / update the value of the cell
        if (!is_already_rendered || update_value_cell) {
                int value_cell_width = value_text_max_len * fw + 2 * h_padding;
                int value_cell_height = fh + v_padding;
                // Here we redraw the entire value cell (with the white
                // background so it is quite fast) If we need to render more
                // text we could make it more optimised to only redraw over the
                // place where actual characters are printed.
                if (customization->rendering_mode == Detailed) {
                        display->draw_rounded_rectangle(
                            value_cell_start, value_cell_width,
                            value_cell_height, value_cell_height / 2, White);
                        display->draw_string(
                            {.x = value_cell_start.x + h_padding,
                             .y = value_cell_start.y + value_cell_v_padding},
                            (char *)value_text, Size16, White, Black);
                } else {
                        // The only other option supported right now is the
                        // `Minimalistic` rendering mode, we render it below
#ifdef EMULATOR
                        // We need to clear the background in black so that it
                        // is the previous text is erased. Note that this is
                        // only required on the emulator as the actual LCD
                        // display always clears the background of the text.
                        display->draw_rectangle(
                            value_cell_start, value_cell_width,
                            value_cell_height, Black, 0, true);
#endif
                        display->draw_rectangle(
                            value_cell_start, value_cell_width,
                            value_cell_height, accent_color, 1, false);
                        display->draw_string(
                            {.x = value_cell_start.x + h_padding,
                             .y = value_cell_start.y + value_cell_v_padding},
                            (char *)value_text, Size16, Black, White);
                }
        }
}
/**
 * Similar to `render_config_bar_centered` refer to the documentation above
 * to understand the purpose of the function. The only difference is that this
 * is used for bars that have no value cells (e.g. bars displaying the name of
 * the game or the start 'button' bars).
 *
 * Given that there is on value cell that would require a re-render. This
 * function takes in only the `is_already_rendered` parameter. This is to be
 * correctly handled by the caller. That is, the function should be called with
 * this param as true only once per configuration collection session. The idea
 * is that since this text bar doesn't change, it makes no sense to re-render it
 * every time.
 */
void render_text_bar_centered(Display *display, int y_start,
                              int option_text_max_len, int value_text_max_len,
                              const char *text, bool is_already_rendered,
                              UserInterfaceRenderingMode rendering_mode,
                              Color background_color, Color text_color,
                              int font_width, FontSize font_size)
{

        // We calculate the total width using the same logic as for the config
        // bars. The reason we accept the same parameters is that the text bars
        // are to be created in the same context as the config bars so we'll
        // already have those max len params calculated there.
        int option_value_gap_len = 2;
        int text_len =
            option_text_max_len + option_value_gap_len + value_text_max_len;

        int h = display->get_height();
        int w = display->get_width();
        int fw = font_width;
        int fh = font_size;

        int left_margin = get_centering_margin(w, fw, text_len);

        int text_x = get_centering_margin(w, fw, strlen(text));

        // We determine centering of the configuration bars based on the
        // position of the text. The actual rounded rect that contains the text
        // is slightly larger. Because of this we add 'padding' to the config
        // bar by making it start slightly to the left and up from the text. We
        // then make it longer and taller to properly contain all of the text.
        int h_padding = fw / 2;
        int v_padding = fh / 2;
        Point bar_start = {.x = left_margin - h_padding,
                           .y = y_start - v_padding};

        int bar_width = text_len * fw + 2 * h_padding;

        if (!is_already_rendered) {
                Point text_start = {.x = text_x, .y = y_start};
                if (rendering_mode == Detailed) {
                        // Draw the background for the two configuration cells.
                        display->draw_rounded_rectangle(
                            bar_start, bar_width, fh * 2, fh, background_color);
                        // Draw the actual text of the text bar. Note that it
                        // will be centered inside of the large bar.
                        display->draw_string(
                            text_start, (char *)text, font_size,
                            background_color,
                            get_good_contrast_text_color(background_color));
                } else {

#ifdef EMULATOR
                        // We need to clear the background in black so that it
                        // is the previous text is erased. Note that this is
                        // only required on the emulator as the actual LCD
                        // display always clears the background of the text.
                        display->clear_region(
                            bar_start,
                            {bar_start.x + bar_width, bar_start.y + fh * 2},
                            Black);
#endif
                        // The only other option supported right now is the
                        // `Minimalistic` rendering mode, we render it below
                        // Draw the background for the two configuration cells.
                        display->draw_rectangle(bar_start, bar_width, fh * 2,
                                                background_color, 1, false);

                        // Draw the actual text of the text bar. Note that it
                        // will be centered inside of the large bar.
                        display->draw_string(text_start, (char *)text,
                                             font_size, Black, text_color);
                }
        }
}

/**
 * Renders a small circle indicator in one of n given vertical positions. This
 * is used to indicate which config option is currently being edited. The
 * caller is responsible for calculating the positions of the circle indicators
 * and supplying the index of the previously selected position and the new one.
 *
 * The caller is responsible for correctly setting the `y_positions_len` to
 * match the supplied pointer to the `y_positions`, otherwise the function will
 * error with invalid array access.
 */
void render_circle_selector(Display *display, bool already_rendered, int x_axis,
                            int *y_positions, int y_positions_len,
                            int prev_pos_idx, int curr_pos_idx, int radius,
                            Color bg_color, Color circle_color)
{
        // We ignore the array overflow.
        if (prev_pos_idx >= y_positions_len ||
            curr_pos_idx >= y_positions_len) {
                return;
        }
        if (!already_rendered || prev_pos_idx != curr_pos_idx) {
                // First clear the old circle
                Point clear_pos = {.x = x_axis, .y = y_positions[prev_pos_idx]};
                display->draw_circle(clear_pos, radius, bg_color, 0, true);

                // Draw the new circle
                Point new_pos = {.x = x_axis, .y = y_positions[curr_pos_idx]};
                display->draw_circle(new_pos, radius, circle_color, 0, true);
        }
}

/**
 * Calculates the amount of spacing required so that the 3 following spacings
 * are equal:
 * - space from the top of the screen to the game / config title (heading)
 * - space from the heading to the config bars
 * - space from the config bars to the bottom of the screen.
 * We do this by taking the total height, subtacting the combined
 * height of all config bars + the game title (heading) and dividing
 * by 3 (the number of spacings)
 * inputs
 */
int calculate_section_spacing(int display_height, int config_bar_num,
                              int bar_height, int gap_between_bars_height,
                              FontSize heading_font_size)
{
        int spacings_num = 3;
        int total_gaps = config_bar_num - 1;
        int config_bars_height = config_bar_num * bar_height;
        int total_gaps_height = total_gaps * gap_between_bars_height;
        int total_config = config_bars_height + total_gaps_height;
        // Having calculated all intermediate heights, we get the final spacing.
        return (display_height - total_config - (int)heading_font_size) /
               spacings_num;
}
/**
 * Given the initial spacing in front of the config heading and the number,
 * sizes and gap size between the config bars, calculates the array of their y
 * positions and returns a pointer to it.
 *
 * The caller is responsible for freeing up this memory after the positions are
 * used.
 *
 */
int *calculate_config_bar_positions(int y_spacing, FontSize heading_font_size,
                                    int bar_height, int bar_gap_height,
                                    int config_bar_num)
{
        int heading_end = y_spacing + heading_font_size;

        int *bar_positions = (int *)malloc(config_bar_num * sizeof(int));

        int curr_bar_y = heading_end + y_spacing;
        for (int i = 0; i < config_bar_num; i++) {
                bar_positions[i] =
                    curr_bar_y + (bar_height + bar_gap_height) * i;
        }
        return bar_positions;
}

inline int get_centering_margin(int screen_width, int font_width,
                                int text_length)
{
        return (screen_width - text_length * font_width) / 2;
}

/**
 *
 * @param `text_update_only` controlls if the config menu has already been
 * rendered for the first time and only the text sections require updating. This
 * is required on the physical lcd display because redrawing the entire menu
 * every time is too slow so we need to be efficient about it.
 */
void render_config_menu(Display *display, Configuration *config,
                        ConfigurationDiff *diff, bool text_update_only,
                        UserInterfaceCustomization *customization,
                        bool should_render_logo)
{
        int max_option_name_length =
            find_max_config_option_name_text_length(config);
        int max_option_value_length =
            std::min(find_max_config_option_value_text_length(config),
                     MAX_CONFIG_OPTION_VALUE_LENGTH);
        int text_max_length =
            max_option_name_length + max_option_value_length + 1;

        LOG_DEBUG(TAG, "Found max text length across all config bars: %d",
                  text_max_length);
        int spacing = (display->get_height() - config->options_len * FONT_SIZE -
                       HEADING_FONT_SIZE) /
                      3;

        const char *heading_text = config->name;

        // We exctract the display dimensions and font sizes into shorter
        // variable names to make the code easier to read.
        int h = display->get_height();
        int w = display->get_width();
        int fw = FONT_WIDTH;
        int fh = FONT_SIZE;
        int left_margin = get_centering_margin(w, fw, text_max_length);

        int bars_num = config->options_len;

        int bar_height = 2 * fh;
        int bar_gap_height = fh * 3 / 4;
        int y_spacing = calculate_section_spacing(h, bars_num, bar_height,
                                                  bar_gap_height, Size24);

        if (!text_update_only) {
                display->initialize();
                display->clear(Black);
        }

        int *bar_positions = calculate_config_bar_positions(
            y_spacing, Size24, bar_height, bar_gap_height, bars_num);

        // Render the config menu heading.
        render_text_bar_centered(display, y_spacing, text_max_length, 0,
                                 heading_text, text_update_only,
                                 customization->rendering_mode, Black, White,
                                 HEADING_FONT_WIDTH, Size24);

        if (!text_update_only && should_render_logo) {
                render_logo(display, customization, {.x = 10, .y = y_spacing});
        }

        for (int i = 0; i < config->options_len; i++) {
                int bar_y = bar_positions[i];
                char option_value_buff[max_option_value_length+1];

                ConfigurationOption value = *config->options[i];
                const char *option_text = value.name;

                switch (value.type) {
                case INT: {
                        int selected_value = static_cast<int *>(
                            value.available_values)[value.currently_selected];
                        char format_string[4];
                        sprintf(format_string, "%%%dd",
                                max_option_value_length);
                        sprintf(option_value_buff, format_string,
                                selected_value);
                        break;
                }
                case STRING: {
                        char *selected_value = static_cast<char **>(
                            value.available_values)[value.currently_selected];
                        if (strlen(selected_value) > max_option_value_length) {
                                // We need to truncate the string to fit
                                // in the value cell.
                                int remainder_length =
                                    max_option_value_length - 3;
                                strncpy(option_value_buff, selected_value,
                                        remainder_length);
                                strncpy(option_value_buff + remainder_length,
                                        "...", 3);
                                option_value_buff[max_option_value_length] =
                                    '\0'; // Null terminate
                                break;
                        }
                        char format_string[10];
                        sprintf(format_string, "%%%ds",
                                max_option_value_length);
                        sprintf(option_value_buff, format_string,
                                selected_value);
                        break;
                }
                case COLOR: {
                        Color selected_value = static_cast<Color *>(
                            value.available_values)[value.currently_selected];
                        char format_string[10];
                        sprintf(format_string, "%%%ds",
                                max_option_value_length);
                        sprintf(option_value_buff, format_string,
                                color_to_string(selected_value));
                        break;
                }
                }
                render_config_bar_centered(
                    display, bar_y, max_option_name_length,
                    max_option_value_length, option_text, option_value_buff,
                    text_update_only,
                    std::find(diff->modified_options.begin(),
                              diff->modified_options.end(),
                              i) != diff->modified_options.end(),
                    customization);
                LOG_DEBUG(TAG,
                          "Rendered config bar %d with option text '%s' and "
                          "value '%s'",
                          i, option_text, option_value_buff);
        }

        // Before we render the indicator dot we need to calculate its
        // positions. Note that the dot needs to appear exactly on the middle
        // axis of the config bars, because of this we need to add the
        // horizontal padding to the y positions of the config bars to center
        // the dot. This is to be considered for refactoring but currently this
        // pattern is not crystalized enough to abstract it.
        int padding = 1; // 0.5 fw on either side
        int bar_width = (text_max_length + padding) * fw;
        int right_margin = display->get_width() - (left_margin + bar_width);
        int circle_x = left_margin + bar_width + right_margin / 2;
        int v_padding = fh / 2;
        int circle_ys_len = bars_num;
        int circle_ys[circle_ys_len];
        for (int i = 0; i < circle_ys_len; i++) {
                circle_ys[i] = bar_positions[i] + v_padding;
        }

        render_circle_selector(
            display, text_update_only, circle_x, circle_ys, circle_ys_len,
            diff->previously_edited_option, diff->currently_edited_option,
            SELECTOR_CIRCLE_RADIUS, Black, customization->accent_color);

        free(bar_positions);
}

void render_controls_explanations(Display *display,
                                  std::map<Action, std::string> button_hints)
{
        std::vector<Color> button_colors(4);
        button_colors[Action::BLUE] = Blue;
        button_colors[Action::YELLOW] = Yellow;
        button_colors[Action::RED] = Red;
        button_colors[Action::GREEN] = Green;

        int h = display->get_height();
        int w = display->get_width();
        int fw = FONT_WIDTH;
        int fh = FONT_SIZE;

        // Dynamically find the total text length needed for even spacing
        int total_text_len = 0;
        for (auto textMapping : button_hints) {
                total_text_len += textMapping.second.length();
        }

        int circle_radius = 2;

        // Given that the help text is rendred at the bottom and the screen
        // has rounded corners, we need to set a fixed margin to ensure that
        // nothing is cropped by the corners.
        int x_margin = 2 * fw;

        int circle_text_gap_width = fw / 4;
        int total_len_to_render =
            button_hints.size() * (circle_radius + circle_text_gap_width) +
            fw * total_text_len + 2 * x_margin;

        int remainder_width = w - total_len_to_render;
        int gaps = button_hints.size() - 1;

        int gap_size = remainder_width / gaps;

        // This is empiricaly calibrated to look nice. It is set as a negative
        // offset from the bottom of the screen and does not depend on what is
        // rendered above.
        int help_text_y = h - 3 * fh / 2;
        // Also eye-callibrated, not much logic to the 3/4 * fh.
        int circle_indicator_y = help_text_y + 3 * fh / 4;

#ifndef EMULATOR
        // The font on the emulator differs slightly from the target
        // LCD display font, so we need to apply this vertical alignment
        // override.
        help_text_y += fh / 4;
#endif

        std::vector<Action> buttons_order = {Action::BLUE, Action::YELLOW,
                                             Action::GREEN, Action::RED};

        // We keep track of the current x position as we render hint items.
        int x_pos = x_margin;
        for (int i = 0; i < buttons_order.size(); i++) {
                Action button = buttons_order[i];
                std::string hint = button_hints[button];
                Color color = button_colors[button];
                display->draw_circle({.x = x_pos, .y = circle_indicator_y},
                                     circle_radius, color, 0, true);
                x_pos += circle_radius + circle_text_gap_width;
                display->draw_string({.x = x_pos, .y = help_text_y},
                                     (char *)hint.c_str(), FontSize::Size16,
                                     Black, White);

                x_pos += hint.length() * fw;
                x_pos += gap_size;
        }
}

/**
 * Renders the default explanation of console UI controls.
 */
void render_controls_explanations(Display *display)
{

        std::map<Action, std::string> button_hints;
        button_hints[Action::BLUE] = "Back";
        button_hints[Action::YELLOW] = "Help";
        button_hints[Action::RED] = "Next";
        button_hints[Action::GREEN] = "Toggle";

        render_controls_explanations(display, button_hints);
}

void render_wrapped_text(Platform *p, UserInterfaceCustomization *customization,
                         const char *text)
{
        p->display->clear(Black);

        // We exctract the display dimensions and font sizes into shorter
        // variable names to make the code easier to read.
        int h = p->display->get_height();
        int w = p->display->get_width();
        int margin = p->display->get_display_corner_radius();
        int fw = FONT_WIDTH;
        int fh = FONT_SIZE;

        // We allow the text to go into 1/2 of the width of the display corner
        // radius
        int maximum_line_chars = (w - margin) / fw;

        int text_len = strlen(text);
        int text_x = margin / 2;
        int text_start_y = 2 * fh;

        int lines_drawn = 0;
        int curr_word_x_offset = 0;
        bool first_word = true;

        // We allocate dynamically a copy of the constant string as strtok
        // needs a mutable reference.
        char *text_copy = (char *)calloc(strlen(text) + 1, sizeof(char));
        // We need to properly null-terminate the other string.
        text_copy[strlen(text)] = '\0';
        strncpy(text_copy, text, strlen(text));

        char *word = strtok((char *)text_copy, " ");
        while (word != nullptr) {
                int curr_y = text_start_y + fh * lines_drawn;
                if (curr_word_x_offset + strlen(word) + 1 <=
                    maximum_line_chars) {
                        if (first_word) {
                                // We omit the space separator on the first
                                // word.
                                first_word = false;
                        } else {
                                curr_word_x_offset += 1;
                        }
                } else {
                        lines_drawn++;
                        curr_word_x_offset = 0;
                        curr_y = text_start_y + fh * lines_drawn;
                }

                p->display->draw_string(
                    {.x = text_x + fw * curr_word_x_offset, .y = curr_y},
                    (char *)word, FontSize::Size16, Black, White);
                curr_word_x_offset += strlen(word);

                word = strtok(nullptr, " ");
        }

        free(text_copy);
}

/**
 * Renders a single block of wrapped text and a guide indicator saying that
 * pressing green will dismiss the help text.
 */
void render_wrapped_help_text(Platform *p,
                              UserInterfaceCustomization *customization,
                              const char *help_text)
{
        render_wrapped_text(p, customization, help_text);

        // We exctract the display dimensions and font sizes into shorter
        // variable names to make the code easier to read.
        int h = p->display->get_height();
        int w = p->display->get_width();
        int margin = p->display->get_display_corner_radius();
        int fw = FONT_WIDTH;
        int fh = FONT_SIZE;

        // We render the part saying that ok closes the help screen
        const char *ok = "OK";
        int ok_text_len = strlen(ok);

        int ok_text_x = w - fw * (ok_text_len + 3);
        int ok_text_y = h - 2 * fh;

        int ok_green_circle_x = ok_text_x + (ok_text_len + 1) * fw;
        int ok_green_circle_y = ok_text_y + 3 * fh / 4;

        // The font on the emulator differs slightly from the target
        // LCD display font, so we need to apply this vertical alignment
        // override.
#ifndef EMULATOR
        ok_text_y += fh / 4;
#endif

        p->display->draw_string({.x = ok_text_x, .y = ok_text_y}, (char *)ok,
                                FontSize::Size16, Black, White);

        // This might need simplification in the future
        int radius = FONT_SIZE / 4;
        int d = 2 * radius;
        int circle_radius = 5;
        p->display->draw_circle(
            {.x = ok_green_circle_x, .y = ok_green_circle_y}, circle_radius,
            Green, 0, true);
}

void draw_cube_perspective(Display *display, Point position, int size,
                           Color color)
{
        display->draw_rectangle(position, size, size, color, 1, false);

        // When drawing rectangles and lines the positions are slightly
        // misaligned and don't look pixel-accurate. We need to adjust the
        // starting positions of the vertices to align with the rectangle
        // vertices precisely. This is only required on the emulator. I suspect
        // this is due to the slightly different implementations of drawing
        // lines between the display libraries. In the future this could be
        // moved to the emulator display library.

#ifdef EMULATOR
        int alignment_offset = 1;
#endif
#ifndef EMULATOR
        int alignment_offset = 0;
#endif

        Point front_top_left_vertex = {position.x - alignment_offset,
                                       position.y};
        Point front_top_right_vertex = {position.x + size, position.y};
        Point front_bottom_right_vertex = {
            position.x + size, position.y + size + alignment_offset};
        auto front_vertices = {front_top_left_vertex, front_top_right_vertex,
                               front_bottom_right_vertex};

        // Draw the three visible slanted edges
        int perspective_offset = size / 3;

        auto translate_to_back = [&perspective_offset](Point vertex) {
                return Point{vertex.x + perspective_offset,
                             vertex.y - perspective_offset};
        };

        for (Point vertex : front_vertices) {
                display->draw_line(vertex, translate_to_back(vertex), color);
        }

        // Draw the two visible back edges
        Point back_top_left_vertex = translate_to_back(front_top_left_vertex);
        Point back_top_right_vertex = translate_to_back(front_top_right_vertex);
        Point back_bottom_right_vertex =
            translate_to_back(front_bottom_right_vertex);

        display->draw_line(back_top_left_vertex, back_top_right_vertex, color);
        display->draw_line(back_top_right_vertex, back_bottom_right_vertex,
                           color);
}
/**
 * Draws a Greek mu letter (μ) contained inside of a square box with edge width
 * equal to `size`. Note that for pixel accuracy the size of the mu letter
 * should be divisible by 6.
 */
void draw_mu_letter(Display *display, Point position, int size, Color color)
{
        int width = size / 3;
        int height = 2 * width;
        int v_margin = (size - height) / 2;
        int h_margin = width;

        // First we draw the 'leg' which is the vertical long part of the μ
        // letter
        Point letter_leg_start = {position.x + h_margin, position.y + v_margin};
        Point letter_leg_end = {letter_leg_start.x,
                                letter_leg_start.y + height};

        display->draw_line(letter_leg_start, letter_leg_end, color);

        // Controls how much the front of the μ letter sticks out from the round
        // part.
        int letter_front_gap = width / 6;
        // Then we draw the front part of the letter
        Point letter_front_start = {position.x + h_margin + width,
                                    position.y + v_margin};
        Point letter_front_end = {letter_front_start.x, letter_front_start.y +
                                                            height / 2 +
                                                            letter_front_gap};

        display->draw_line(letter_front_start, letter_front_end, color);

        // Now we connect the two parts with a semi-circle
        int radius = width / 2 - 1;
        Point center = {letter_leg_start.x + radius,
                        letter_front_end.y - letter_front_gap};
        // For pixel accuracy we decrease the diameter by 1 as the circle
        // also has some thickness to it.
        display->draw_circle(center, radius, color, 1, false);

        // We now clear the top part of the circle to be left with the mu
        // letter only.

        display->clear_region(
            letter_leg_start,
            {letter_front_end.x - 1, letter_front_end.y - letter_front_gap},
            Black);
}

/**
 * Draws an on-screen keyboard and allows the user to move around it using the
 * cursor. The text entered by the user will be returned in a heap-allocated
 * character array. The caller of this function is resposible for deallocating
 * this array once done processing the data. If the user cancels the input
 * process, an empty optional is returned.
 */
std::optional<char *>
collect_string_input(Platform *p, UserInterfaceCustomization *customization,
                     const char *input_prompt)
{
        Display *display = p->display;
        LOG_DEBUG(TAG, "Entered the user string input collection subroutine.");

        // Bind input params to short names for improved readability.
        int w = display->get_width();
        int h = display->get_height();
        int r = display->get_display_corner_radius();

        int margin = r / 4;
        int usable_width = w - margin / 2;
        int usable_height = h - margin / 2;

        int max_cols = usable_width / FONT_WIDTH;
        int max_rows = usable_height / FONT_SIZE;

        int actual_width = max_cols * FONT_WIDTH;
        int actual_height = max_rows * FONT_SIZE;

        // We calculate centering margins
        int left_horizontal_margin = (w - actual_width) / 2;

        int keyboard_rows = 4;
        int input_row = 1;
        int spacing = 2;
        int text_rows = keyboard_rows + input_row + spacing;
        int top_vertical_margin = (h - text_rows * FONT_SIZE) / 2;

        LOG_DEBUG(TAG, "Keyboard grid area has %d available columns", max_cols);

        p->display->clear(Black);

        int prompt_text_centering_margin =
            (w - strlen(input_prompt) * FONT_WIDTH) / 2;
        Point prompt_text_start = {.x = prompt_text_centering_margin,
                                   .y = FONT_WIDTH};

        p->display->draw_string(prompt_text_start, (char *)input_prompt,
                                FontSize::Size16, Black, White);

        // Note how the bottom right part of the keyboard is filled with
        // spaces this is needed to ensure that the selection cursor is
        // translated within a rectangular area and we don't get ouside
        // string buffer index errors. Also note that the 'x' character at the
        // end is a placeholder for cancellation button. If the user selects
        // that, the input process is aborted.
        std::vector<const char *> base_char_map = {
            "`1234567890-=",
            "qwertyuiop[]\\",
            "asdfghjkl;'  ",
            "zxcvbnm,./ x ",
        };

        std::vector<const char *> shift_char_map = {
            "~!@#$%^&*()_+",
            "QWERTYUIOP{}|",
            "ASDDFGHJKL:\" ",
            "ZXCVBNM<>?  x ",
        };
        Point cancellation_key_location = {11, 3};

        std::vector<int> left_indent_map = {0, 1, 2, 3};

        Point input_text_start = {.x = left_horizontal_margin,
                                  .y = top_vertical_margin};
        Point input_text_start_second_line = {.x = left_horizontal_margin,
                                              .y = top_vertical_margin +
                                                   FONT_SIZE + 4};

        // For now we only support up to two lines of user input. Longer strings
        // are not supported.
        int max_input_len = max_cols * 2;
        // We always itialize the buffer with size +1 to allow for the null
        // terminator.
        char *output = (char *)malloc(sizeof(char) * (max_input_len + 1));
        // This is only used for rendering if the length of output string is
        // greater than `max_cols`. Unlike the main output,
        // this buffer is freed at the end and is not returned to the user.
        char *output_line_1 =
            (char *)malloc(sizeof(char) * (max_input_len + 1));
        char *output_line_2 =
            (char *)malloc(sizeof(char) * (max_input_len + 1));
        // If the user enters nothing we need to be safe and still write the
        // null terminator.
        output[0] = '\0';

        Point cursor = {0, 0};

        int keyboard_start_y =
            top_vertical_margin + (input_row + spacing) * FONT_SIZE;

        auto render_character_at_location =
            [display, &cursor, left_indent_map,
             keyboard_start_y](Point location, Color color,
                               std::vector<const char *> &character_map) {
                    int x = location.x;
                    int y = location.y;
                    const char *row = character_map[y];
                    int left_indent = left_indent_map[y];
                    // we mutiply the index by two here to spread out the
                    // keyboard characters a bit.
                    Point start = {.x = (left_indent + 2 * x) * FONT_WIDTH,
                                   .y = keyboard_start_y + y * FONT_SIZE};
                    char buffer[2];
                    buffer[0] = row[x];
                    buffer[1] = '\0';
                    display->clear_region(
                        start, {start.x + FONT_WIDTH, start.y + FONT_SIZE + 4},
                        Black);
                    display->draw_string(start, buffer, FontSize::Size16, Black,
                                         color);
            };

        // We define this reusasble lambda so that we can easily re-render the
        // grid when the capitalization setting changes.
        auto render_keyboard = [&cursor, customization,
                                render_character_at_location](
                                   std::vector<const char *> &character_map) {
                for (int y = 0; y < character_map.size(); y++) {
                        for (int x = 0; x < strlen(character_map[y]); x++) {
                                Color color = (cursor.x == x && cursor.y == y)
                                                  ? customization->accent_color
                                                  : White;
                                render_character_at_location({x, y}, color,
                                                             character_map);
                        }
                }
        };

        auto extract_current_char =
            [&cursor](std::vector<const char *> &character_map) {
                    return character_map[cursor.y][cursor.x];
            };

        auto render_current_input_text = [display, input_text_start,
                                          input_text_start_second_line,
                                          max_cols, output, output_line_1,
                                          output_line_2](int output_idx) {
                int line_1_end = std::min(output_idx, max_cols);
                // We clear one past the end of the line to ensure that this
                // function also works for re-rendering after backspace is hit.
                display->clear_region(
                    input_text_start,
                    {input_text_start.x + FONT_WIDTH * (line_1_end + 1),
                     input_text_start.y + FONT_SIZE + 4},
                    Black);

                strncpy(output_line_1, output, line_1_end);
                // ensure that the rendered line 1 is properly null-terminated
                output_line_1[line_1_end] = '\0';
                display->draw_string(input_text_start, output_line_1,
                                     FontSize::Size16, Black, White);
                // Only render second line if enough chars
                int line_2_end = output_idx - max_cols;
                if (output_idx >= max_cols) {
                        // We clear even if it is equal to ensure that when this
                        // function is used for re-rendering after backspace,
                        // the last character from the second line disappears.
                        display->clear_region(
                            input_text_start_second_line,
                            {input_text_start_second_line.x +
                                 FONT_WIDTH * (line_2_end + 1),
                             input_text_start_second_line.y + FONT_SIZE + 4},
                            Black);
                }
                if (output_idx > max_cols) {
                        strncpy(output_line_2, (output + max_cols), line_2_end);
                        // ensure that the rendered line 2 is properly
                        // null-terminated
                        output_line_2[line_2_end] = '\0';
                        display->draw_string(input_text_start_second_line,
                                             output_line_2, FontSize::Size16,
                                             Black, White);
                }
        };

        render_keyboard(base_char_map);

        if (customization->show_help_text) {
                std::map<Action, std::string> button_hints;
                button_hints[Action::BLUE] = "Erase";
                button_hints[Action::YELLOW] = "Caps";
                button_hints[Action::RED] = "Done";
                button_hints[Action::GREEN] = "Select";
                render_controls_explanations(p->display, button_hints);
        }

        bool input_confirmed = false;
        bool is_capitalized = false;
        auto curr_char_map = base_char_map;
        int output_idx = 0;
        while (!input_confirmed) {
                Direction dir;
                Action act;
                if (directional_input_registered(p->directional_controllers,
                                                 &dir)) {
                        render_character_at_location(cursor, White,
                                                     curr_char_map);
                        translate_toroidal_array(&cursor, dir,
                                                 curr_char_map.size(),
                                                 strlen(base_char_map[0]));
                        render_character_at_location(
                            cursor, customization->accent_color, curr_char_map);
                        p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
                }
                if (action_input_registered(p->action_controllers, &act)) {
                        switch (act) {
                        case YELLOW: {
                                is_capitalized = !is_capitalized;
                                curr_char_map = is_capitalized ? shift_char_map
                                                               : base_char_map;
                                render_keyboard(curr_char_map);
                                break;
                        }
                        case RED:
                                input_confirmed = true;
                                break;
                        case GREEN: {
                                if (cursor.x == cancellation_key_location.x &&
                                    cursor.y == cancellation_key_location.y) {
                                        LOG_DEBUG(
                                            TAG,
                                            "User selected the cancellation "
                                            "key, aborting input collection.");
                                        free(output);
                                        free(output_line_1);
                                        free(output_line_2);
                                        return std::nullopt;
                                }
                                if (output_idx < max_input_len) {
                                        char selection =
                                            extract_current_char(curr_char_map);
                                        output[output_idx] = selection;
                                        // We need to null-terminate the
                                        // unfinished input to be able to print
                                        // it.
                                        output[output_idx + 1] = '\0';
                                        output_idx++;
                                        render_current_input_text(output_idx);
                                }
                                break;
                        }
                        case BLUE:
                                if (output_idx > 0) {
                                        output_idx--;
                                        output[output_idx] = '\0';
                                        render_current_input_text(output_idx);
                                }
                                break;
                        }
                        p->delay_provider->delay_ms(MOVE_REGISTERED_DELAY);
                }
                p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
                p->display->refresh();
        }

        free(output_line_2);
        return output;
}

void render_logo(Display *display, UserInterfaceCustomization *customization,
                 Point position)
{

        int size = 24;
        draw_cube_perspective(display, position, size,
                              customization->accent_color);
        draw_mu_letter(display, position, size, customization->accent_color);
}

void wait_until_green_pressed(Platform *p)
{
        while (true) {
                Action act;
                if (action_input_registered(p->action_controllers, &act)) {
                        if (act == Action::GREEN) {
                                LOG_DEBUG(TAG, "User confirmed 'OK'");
                                p->delay_provider->delay_ms(
                                    MOVE_REGISTERED_DELAY);
                                return;
                        }
                }
                p->delay_provider->delay_ms(INPUT_POLLING_DELAY);
                p->display->refresh();
        }
}
