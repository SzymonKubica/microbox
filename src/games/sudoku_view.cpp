#include "sudoku_view.hpp"
#include <cassert>
#include <functional>
#include <optional>
#include "../common/grid.hpp"
#include "../common/constants.hpp"
#include "../common/maths_utils.hpp"
#include "sudoku_engine.hpp"

/* Grid Cell Rendering */

/**
 * Renders the grid 'frame' that separates individual grid frames.
 */
void SimpleSudokuView::render_grid()
{
        display->initialize();
        display->clear(Black);

        // Assign shorter names to improve readability.
        int x_margin = dimensions.left_horizontal_margin;
        int y_margin = dimensions.top_vertical_margin;
        int w = dimensions.actual_width;
        int h = dimensions.actual_height;
        Color color = customization.accent_color;

        int cell_size = dimensions.actual_height / 9;

        // We only render borders between cells for a minimalistic look
        for (int i = 1; i < 9; i++) {
                int offset = i * cell_size;
                auto draw_vertical_line = [&](int offset) {
                        Point start = {x_margin + offset, y_margin};
                        Point end = start + Point{0, w};
                        display->draw_line(start, end, color);
                };
                auto draw_horizontal_line = [&](int offset) {
                        Point start = {x_margin, y_margin + offset};
                        Point end = start + Point{w, 0};
                        display->draw_line(start, end, color);
                };

                draw_vertical_line(offset);
                draw_horizontal_line(offset);

                // We make the lines between big squares thicker by drawing
                // it three times.
                if (i % 3 == 0) {
                        draw_vertical_line(offset - 1);
                        draw_horizontal_line(offset - 1);
                        draw_vertical_line(offset + 1);
                        draw_horizontal_line(offset + 1);
                }
        }
}

/* Helper functions for rendering grid cells */

/**
 * Iterates over all cells in the grid and applies a processor function
 * to all of them.
 */
void process_all_cells(
    const SudokuGrid &grid,
    std::function<void(const SudokuCell &, const Point &)> processor);
void render_digit(Display *display, UserInterfaceCustomization *customization,
                  SquareCellGridDimensions *dimensions, const Point &location,
                  const SudokuCell &value,
                  std::optional<Color> color_override = std::nullopt);
void erase_digit(Display *display, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location);
void render_digit_underline(Display *display,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions,
                            Point location);

void SimpleSudokuView::render_grid_numbers(const SudokuGrid &grid)
{
        auto render_if_value_present = [&](const SudokuCell &cell,
                                           const Point &location) {
                if (cell.digit.has_value())
                        render_digit(display, &customization, &dimensions,
                                     location, cell);
        };
        process_all_cells(grid, render_if_value_present);
}

void SimpleSudokuView::underline_all_instances(int digit,
                                               const SudokuGrid &grid)
{
        auto underline_if_value_present = [&](const SudokuCell &cell,
                                              const Point &location) {
                if (cell.digit.has_value() && cell.digit.value() == digit)
                        render_digit_underline(display, &customization,
                                               &dimensions, location);
        };
        process_all_cells(grid, underline_if_value_present);
}
void SimpleSudokuView::remove_underline_all_instances(int digit,
                                                      const SudokuGrid &grid)
{
        auto remove_underline_if_value_present = [&](const SudokuCell &cell,
                                                     const Point &location) {
                if (cell.digit.has_value() && cell.digit.value() == digit) {
                        erase_cell_contents(location);
                        render_digit(display, &customization, &dimensions,
                                     location, cell);
                }
        };
        process_all_cells(grid, remove_underline_if_value_present);
}
void process_all_cells(
    const SudokuGrid &grid,
    std::function<void(const SudokuCell &, const Point &)> processor)
{
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        const auto &cell = grid[y][x];
                        const Point location = {x, y};
                        processor(cell, location);
                }
        }
}

/**
 * Given the grid dimensions and a location on the grid, it calculates the
 * exact start location where the text inside of the cell should be rendered.
 */
Point calculate_cell_text_start(SquareCellGridDimensions *dimensions,
                                const Point &location)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
        int x_offset = location.x * cell_size;
        int y_offset = location.y * cell_size;

        // To ensure that the characters are centered inside of each
        // cell, we need to calculate the 'padding' around each
        // character. This is defined by the font height and width. Note
        // that we are subtracting one to take the cell border into
        // account and make it visually centered.
        int border_adjustment_offset = 1;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;

        int x_padding = (cell_size - fw) / 2 - border_adjustment_offset;
        int y_padding = (cell_size - fh) / 2 - border_adjustment_offset;

        // Because of pixel-precision inaccuracies we need to adjust the
        // placement of the numbers in the grid.
#ifdef EMULATOR
        int adj = 0;
#else
        int adj = 1;
#endif
        int x = x_margin + x_offset + x_padding + adj;
        int y = y_margin + y_offset + y_padding + adj;

        return {x, y};
}
void render_digit(Display *display, UserInterfaceCustomization *customization,
                  SquareCellGridDimensions *dimensions, const Point &location,
                  const SudokuCell &cell, std::optional<Color> color_override)
{

        assert(cell.digit.has_value() &&
               "Only cells with values should be rendered");

        Point start = calculate_cell_text_start(dimensions, location);
        Color render_color = cell.is_user_defined ? White : Gray;

        if (color_override.has_value()) {
                render_color = color_override.value();
        }

        char buffer[2];
        sprintf(buffer, "%d", cell.digit.value());
        display->draw_string(start, buffer, FontSize::Size16, Black,
                             render_color);
}

void render_digit_underline(Display *display,
                            UserInterfaceCustomization *customization,
                            SquareCellGridDimensions *dimensions,
                            Point location)
{
        Point start = calculate_cell_text_start(dimensions, location);
        int x = start.x;
        int y = start.y;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;
        display->draw_line({x, y + fh - 1}, {x + fw, y + fh - 1},
                           customization->accent_color);
        display->draw_line({x, y + fh}, {x + fw, y + fh},
                           customization->accent_color);
}

void erase_digit(Display *display, UserInterfaceCustomization *customization,
                 SquareCellGridDimensions *dimensions, Point location)
{

        Point start = calculate_cell_text_start(dimensions, location);
        int x = start.x;
        int y = start.y;
        int fh = FONT_SIZE;
        int fw = FONT_WIDTH;

        // For active numbers that were underlined we need to erase a bit
        // further down to remove the underline.
        int underline_adj = 1;

        // Because of pixel-precision inaccuracies we need to adjust the
        // placement of the numbers in the grid.
#ifdef EMULATOR
        int adj = 0;
#else
#if defined(WAVESHARE_2_4_INCH_LCD)
        // TODO: make this consistent across different displays.
        int adj = 0;
#else
        int adj = 1;
#endif
#endif

        display->clear_region({x, y}, {x + fw + adj, y + fh + underline_adj},
                              Black);
}

void SimpleSudokuView::render_cell(const SudokuCell &cell,
                                   const Point &location)
{
        render_digit(display, &customization, &dimensions, location, cell);
}
void SimpleSudokuView::underline_cell(const Point &location)
{
        render_digit_underline(display, &customization, &dimensions, location);
}
void SimpleSudokuView::erase_cell_contents(const Point &location)
{
        erase_digit(display, &customization, &dimensions, location);
}

/* User-controlled Caret Rendering */

/**
 * Renders the caret - a small square telling the user which cell they are
 * currently looking at.
 */
void draw_caret(Display *display, SquareCellGridDimensions *dimensions,
                const Point &caret, Color caret_color)
{

        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
        // The caret needs to be drawn inside of the cell and avoid
        // touching its borders. Because of this we need to shift the
        // caret inside the grid and make its sides appropriately
        // shorter.
        int offset = 3;

        int start_x = x_margin + cell_size * caret.x + offset;
        int start_y = y_margin + cell_size * caret.y + offset;

        int caret_width = cell_size - 2 * offset;

        display->draw_rectangle({start_x, start_y}, caret_width, caret_width,
                                caret_color, 1, false);
}

void SimpleSudokuView::render_caret(const Point &location)
{
        draw_caret(display, &dimensions, location, White);
}

void SimpleSudokuView::erase_caret(const Point &location)
{
        draw_caret(display, &dimensions, location, Black);
}
void SimpleSudokuView::move_caret(const Point &from, const Point &to)
{
        this->erase_caret(from);
        this->render_caret(to);
}
void SimpleSudokuView::rerender_caret(const Point &location)
{
        this->erase_caret(location);
        this->render_caret(location);
}

/* Active Digit Selector */

/**
 * Renders a list of available numbers to the left of the main sudoku grid.
 * It is used to change the currently active digit.
 */
void render_digit_selection_indicator(Display *display,
                                      SquareCellGridDimensions *dimensions,
                                      int selected_number, Color color)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
        int x_offset = -1.5 * cell_size;
        int y_offset = (selected_number - 1) * cell_size;
        int circle_radius = 3;

        int padding = (cell_size - circle_radius) / 2;
        // Need this to make the selector visually balanced and
        // centered.
#ifdef EMULATOR
        int y_adjustment = 3;
#else
        int y_adjustment = 1;
#endif

        int x = x_margin + x_offset + padding;
        int y = y_margin + y_offset + padding + y_adjustment;

        display->draw_circle({x, y}, circle_radius, color, 1, true);
}

void erase_active_digit_selection_indicator(
    Display *display, SquareCellGridDimensions *dimensions, int selected_number)
{
        render_digit_selection_indicator(display, dimensions, selected_number,
                                         Black);
}
void SimpleSudokuView::move_active_digit_selection_indicator(
    int previous_digit, int current_active_digit)
{
        erase_active_digit_selection_indicator(display, &dimensions,
                                               previous_digit);
        render_digit_selection_indicator(display, &dimensions,
                                         current_active_digit,
                                         customization.accent_color);
}

void SimpleSudokuView::render_active_digit_selection_indicator(int active_digit)
{
        render_digit_selection_indicator(display, &dimensions, active_digit,
                                         customization.accent_color);
}

void SimpleSudokuView::render_active_digit_selector()
{
        for (int i = 0; i < 9; i++) {
                SudokuCell cell(i + 1, true);
                // Here we repurpose the number drawing functionality to
                // render available numbers two cells to the left of the main
                // grid. We set the active number to 0 to ensure that the
                // indicator does not get rendered.
                render_digit(display, &customization, &dimensions, {-2, i},
                             cell);
        }
}

void SimpleSudokuView::mark_digit_completed(int digit)
{
        int digit_idx = digit - 1;
        SudokuCell cell(digit, true);
        // Here we repurpose the number drawing functionality to
        // render available numbers two cells to the left of the main
        // grid. We set the active number to 0 to ensure that the
        // indicator does not get rendered.
        Point location = {-2, digit_idx};
        erase_digit(display, &customization, &dimensions, location);
        render_digit(display, &customization, &dimensions, location, cell,
                     customization.accent_color);
}
void SimpleSudokuView::unmark_digit_completed(int digit)
{
        int digit_idx = digit - 1;
        SudokuCell cell(digit, true);
        // Here we repurpose the number drawing functionality to
        // render available numbers two cells to the left of the main
        // grid. We set the active number to 0 to ensure that the
        // indicator does not get rendered.
        Point location = {-2, digit_idx};
        erase_digit(display, &customization, &dimensions, location);
        render_digit(display, &customization, &dimensions, location, cell);
}
