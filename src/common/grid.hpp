#pragma once
#include "platform/interface/platform.hpp"
#include "user_interface_customization.hpp"

/**
 * Stores all information required for rendering the finite grid for game of
 * life simulation. Note that this is similar to the struct that we are using
 * for Minesweeper so we might want to abstract away this commonality in the
 * future.
 *
 * Note: this is currently used for the snake grid as well. In the future this
 * should be moved to a shared module. TODO: remove and move to shared module once
 * Snake prototype is done. (Note 2: Minesweeper also uses the same thing).
 */
typedef struct GameOfLifeGridDimensions {
        int rows;
        int cols;
        int top_vertical_margin;
        int left_horizontal_margin;
        int actual_width;
        int actual_height;

        GameOfLifeGridDimensions(int r, int c, int tvm, int lhm, int aw, int ah)
            : rows(r), cols(c), top_vertical_margin(tvm),
              left_horizontal_margin(lhm), actual_width(aw), actual_height(ah)
        {
        }
} SquareCellGridDimensions;

SquareCellGridDimensions *
calculate_grid_dimensions(int display_width, int display_height,
                          int display_rounded_corner_radius, int game_cell_width);

void draw_grid_frame(Platform *p, UserInterfaceCustomization *customization, SquareCellGridDimensions *dimensions);
