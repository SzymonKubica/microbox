#include "grid.hpp"
#include "logging.hpp"
#include "constants.hpp"

#define TAG "grid"

#ifdef EMULATOR
#define EXPLANATION_ABOVE_GRID_OFFEST 4
#else
#define EXPLANATION_ABOVE_GRID_OFFEST 1
#endif

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius, int cell_width)
{
        // Bind input params to short names for improved readability.
        int w = display_width;
        int h = display_height;
        int r = display_rounded_corner_radius;

        int usable_width = w - r;
        int usable_height = h - r;

        int max_cols = usable_width / cell_width;
        int max_rows = usable_height / cell_width;

        int actual_width = max_cols * cell_width;
        int actual_height = max_rows * cell_width;

        // Margins are required for centering.
        int left_horizontal_margin = (w - actual_width) / 2;
        int top_vertical_margin = (h - actual_height) / 2;

        LOG_DEBUG(TAG,
                  "Calculated grid dimensions: %d rows, %d cols, "
                  "left margin: %d, top margin: %d, actual width: %d, "
                  "actual height: %d",
                  max_rows, max_cols, left_horizontal_margin,
                  top_vertical_margin, actual_width, actual_height);

        return new SquareCellGridDimensions(
            max_rows, max_cols, top_vertical_margin, left_horizontal_margin,
            actual_width, actual_height);
}

void draw_grid_frame(Platform *p, UserInterfaceCustomization *customization,
                     SquareCellGridDimensions *dimensions)

{
        p->display->initialize();
        p->display->clear(Black);

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int actual_width = dimensions->actual_width;
        int actual_height = dimensions->actual_height;

        int border_width = 1;
        // We need to make the border rectangle and the canvas slightly
        // bigger to ensure that it does not overlap with the game area.
        // Otherwise the caret rendering erases parts of the border as
        // it moves around (as the caret intersects with the border
        // partially)
        int border_offset = 2;

        // We draw the border at the end to ensure that it doesn't get cropped
        // by draw string operations above.
        p->display->draw_rectangle(
            {.x = x_margin - border_offset, .y = y_margin - border_offset},
            actual_width + 2 * border_offset, actual_height + 2 * border_offset,
            customization->accent_color, border_width, false);
}

int render_centered_text_above_frame(Platform *p,
                                     SquareCellGridDimensions *dimensions,
                                     char *text)
{

        int y_margin = dimensions->top_vertical_margin;
        int x_margin = dimensions->left_horizontal_margin;

        int available_width = p->display->get_width() - 2 * x_margin;
        // We need to ensure that the border is slightly larger than the
        // drawable grid area so that the border edges don't overlap with the
        // grid area. Otherwise, rendering items in the grid could 'clip' parts
        // of the border.
        int border_offset = 2;

        // Because of slightly different font dimensions, we need this offset
        // override to ensure proper vertical space above the game grid.
        int text_above_grid_y = y_margin - border_offset - FONT_SIZE -
                                EXPLANATION_ABOVE_GRID_OFFEST;

        int text_pixel_len = strlen(text) * FONT_WIDTH;

        int centering_margin = (available_width - text_pixel_len) / 2;

        int toggle_text_x = x_margin + centering_margin;
        p->display->draw_string({.x = toggle_text_x, .y = text_above_grid_y},
                                (char *)text, FontSize::Size16, Black, White);

        return toggle_text_x + text_pixel_len;
}

int render_text_above_frame_starting_from(Platform *p,
                                          SquareCellGridDimensions *dimensions,
                                          char *text, int position,
                                          bool erase_previous)
{
        // We need to ensure that the border is slightly larger than the
        // drawable grid area so that the border edges don't overlap with the
        // grid area. Otherwise, rendering items in the grid could 'clip' parts
        // of the border.
        int border_offset = 2;
        int y_margin = dimensions->top_vertical_margin;
        // Because of slightly different font dimensions, we need this offset
        // override to ensure proper vertical space above the game grid.
        int text_above_grid_y = y_margin - border_offset - FONT_SIZE -
                                EXPLANATION_ABOVE_GRID_OFFEST;

        int text_pixel_len = strlen(text) * FONT_WIDTH;
        if (erase_previous) {
                p->display->clear_region(
                    {.x = position, .y = text_above_grid_y},
                    {.x = position + text_pixel_len,
                     .y = text_above_grid_y + FONT_SIZE},
                    Black);
        }
        p->display->draw_string({.x = position, .y = text_above_grid_y},
                                (char *)text, FontSize::Size16, Black, White);

        return position + text_pixel_len;
}

bool is_out_of_bounds(Point *p, SquareCellGridDimensions *dimensions)
{
        int x = p->x;
        int y = p->y;

        return x < 0 || y < 0 || x >= dimensions->cols || y >= dimensions->rows;
}
