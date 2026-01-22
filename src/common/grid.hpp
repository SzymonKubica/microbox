#pragma once
#include "platform/interface/platform.hpp"
#include "user_interface_customization.hpp"

/**
 * Stores all dimension information required for rendering a grid with square
 * cells.
 */
typedef struct SquareCellGridDimensions {
        int rows;
        int cols;
        int top_vertical_margin;
        int left_horizontal_margin;
        int actual_width;
        int actual_height;

        SquareCellGridDimensions(int r, int c, int tvm, int lhm, int aw, int ah)
            : rows(r), cols(c), top_vertical_margin(tvm),
              left_horizontal_margin(lhm), actual_width(aw), actual_height(ah)
        {
        }
} SquareCellGridDimensions;

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius,
                          int game_cell_width);

void draw_grid_frame(Platform *p, UserInterfaceCustomization *customization,
                     SquareCellGridDimensions *dimensions);

/**
 * Renders text above the grid frame.
 *
 * @return pixel location of the end of the text. Useful for rendering other
 * things behind the centered text (e.g. incrementable score count).
 */
int render_centered_above_frame(Platform *p,
                                     SquareCellGridDimensions *dimensions,
                                     std::string text);
/**
 * Renders text above the grid frame starting from the supplied pixel position
 */
int render_text_above_frame_starting_from(Platform *p,
                                          SquareCellGridDimensions *dimensions,
                                          char *text, int position,
                                          bool erase_previous = false);

bool is_out_of_bounds(Point *p, SquareCellGridDimensions *dimensions);
