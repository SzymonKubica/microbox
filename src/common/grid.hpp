#pragma once
#include "../platform/interface/platform.hpp"
#include "user_interface_customization.hpp"

/**
 * We cannot occupy the entire screen and fill it using the grid. Because of
 * this, we set a default margin that needs to be left on either side of the
 * grid.
 */
#define DEFAULT_MARGIN 20

/**
 * Stores all dimension information required for rendering a grid with square
 * cells.
 */
struct SquareCellGridDimensions {
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
};

constexpr int grid_max_cols(int display_width,
                            int display_rounded_corner_radius, int cell_width)
{
        // If the display does not have rounded corners, we need to ensure that
        // some margin is still there. Note that if the display does have
        // rounded corners, the default margin is the half of that corner radius
        // so that the grid goes into the corners a bit.
        int margin =
            std::max(display_rounded_corner_radius / 2, DEFAULT_MARGIN);
        return (display_width - 2 * margin) / cell_width;
}

constexpr int grid_max_rows(int display_height,
                            int display_rounded_corner_radius, int cell_width)
{
        int margin =
            std::max(display_rounded_corner_radius / 2, DEFAULT_MARGIN);
        return (display_height - 2 * margin) / cell_width;
}

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius,
                          int game_cell_width);

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius, int rows, int cols,
                          bool square_cells);

void draw_grid_frame(const Platform &p,
                     const UserInterfaceCustomization &customization,
                     const SquareCellGridDimensions &dimensions);

/**
 * Renders text above the grid frame.
 *
 * @return pixel location of the end of the text. Useful for rendering other
 * things behind the centered text (e.g. incrementable score count).
 */
int render_centered_above_frame(const Platform &p,
                                const SquareCellGridDimensions &dimensions,
                                std::string text);
/**
 * Renders text above the grid frame starting from the supplied pixel position
 */
int render_text_above_frame_starting_from(
    const Platform &p, const SquareCellGridDimensions &dimensions, char *text,
    int position, bool erase_previous = false);

bool is_out_of_bounds(const Point &p,
                      const SquareCellGridDimensions &dimensions);
