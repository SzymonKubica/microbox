#include "sudoku_view.hpp"
#include <cassert>
#include <functional>
#include <optional>
#include "../common/grid.hpp"
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

        auto draw_vertical_line = [&](int offset, Color color) {
                Point start = {x_margin + offset, y_margin};
                Point end = start + Point{0, w};
                display->draw_line(start, end, color);
        };
        auto draw_horizontal_line = [&](int offset, Color color) {
                Point start = {x_margin, y_margin + offset};
                Point end = start + Point{w, 0};
                display->draw_line(start, end, color);
        };

        // We only render borders between cells for a minimalistic look
        for (int i = 1; i < 9; i++) {
                int offset = i * cell_size;
                draw_vertical_line(offset, color);
                draw_horizontal_line(offset, color);
        }

        // We need to render the gray separators afterwards to ensure that they
        // are on top of the main grid lines rendered using the accent color
        // above.
        for (int i = 3; i < 9; i += 3) {
                int offset = i * cell_size;
                draw_vertical_line(offset, White);
                draw_horizontal_line(offset, White);
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
void render_digit(const Display &display,
                  const UserInterfaceCustomization &customization,
                  const SquareCellGridDimensions &dimensions,
                  const Point &location, const SudokuCell &cell,
                  std::optional<Color> color_override = std::nullopt);
void erase_digit(const Display &display,
                 const UserInterfaceCustomization &customization,
                 const SquareCellGridDimensions &dimensions, Point location);
void render_digit_underline(const Display &display,
                            const UserInterfaceCustomization &customization,
                            const SquareCellGridDimensions &dimensions,
                            Point location);
void erase_digit_underline(const Display &display,
                           const UserInterfaceCustomization &customization,
                           const SquareCellGridDimensions &dimensions,
                           Point location);
void render_digit_underline(const Display &display, Color color,
                            const SquareCellGridDimensions &dimensions,
                            Point location);

void SimpleSudokuView::render_grid_numbers(const SudokuGrid &grid)
{
        auto render_if_value_present = [&](const SudokuCell &cell,
                                           const Point &location) {
                if (cell.digit.has_value())
                        render_digit(*display, customization, dimensions,
                                     location, cell);
        };
        process_all_cells(grid, render_if_value_present);
}

void SimpleSudokuView::underline_all_instances(int digit,
                                               const SudokuGrid &grid,
                                               Color color)
{
        auto underline_if_value_present = [&](const SudokuCell &cell,
                                              const Point &location) {
                if (cell.digit.has_value() && cell.digit.value() == digit)
                        render_digit_underline(*display, color, dimensions,
                                               location);
        };
        process_all_cells(grid, underline_if_value_present);
}
void SimpleSudokuView::underline_all_instances(int digit,
                                               const SudokuGrid &grid)
{
        auto underline_if_value_present = [&](const SudokuCell &cell,
                                              const Point &location) {
                if (cell.digit.has_value() && cell.digit.value() == digit)
                        render_digit_underline(*display, customization,
                                               dimensions, location);
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
                        render_digit(*display, customization, dimensions,
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
Point calculate_cell_text_start(const FontDimensions &font_dimensions,
                                const SquareCellGridDimensions &dimensions,
                                const Point &location)
{
        int x_margin = dimensions.left_horizontal_margin;
        int y_margin = dimensions.top_vertical_margin;

        int cell_size = dimensions.actual_height / 9;
        int x_offset = location.x * cell_size;
        int y_offset = location.y * cell_size;

        // To ensure that the characters are centered inside of each
        // cell, we need to calculate the 'padding' around each
        // character. This is defined by the font height and width. Note
        // that we are subtracting one to take the cell border into
        // account and make it visually centered.
        auto [fw, fh] = font_dimensions;

        int x_padding = (cell_size - fw) / 2;
        int y_padding = (cell_size - fh) / 2;

        int x = x_margin + x_offset + x_padding;
        int y = y_margin + y_offset + y_padding;

        return {x, y};
}
void render_digit(const Display &display,
                  const UserInterfaceCustomization &customization,
                  const SquareCellGridDimensions &dimensions,
                  const Point &location, const SudokuCell &cell,
                  std::optional<Color> color_override)
{

        assert(cell.digit.has_value() &&
               "Only cells with values should be rendered");

        auto font_dimensions = display.get_font_configuration().font_dimensions;
        Point start =
            calculate_cell_text_start(font_dimensions, dimensions, location);
        Color render_color = cell.is_user_defined ? White : Gray;

        if (color_override.has_value()) {
                render_color = color_override.value();
        }

        char buffer[2];
        sprintf(buffer, "%d", cell.digit.value());
        display.draw_string(start, buffer, FontSize::Size16, Black,
                            render_color);
}

void render_digit_underline(const Display &display,
                            const UserInterfaceCustomization &customization,
                            const SquareCellGridDimensions &dimensions,
                            Point location)
{
        render_digit_underline(display, customization.accent_color, dimensions,
                               location);
}

void render_digit_underline(const Display &display, Color color,
                            const SquareCellGridDimensions &dimensions,
                            Point location)
{
        auto font_dimensions = display.get_font_configuration().font_dimensions;
        auto [fw, fh] = font_dimensions;
        Point start =
            calculate_cell_text_start(font_dimensions, dimensions, location);
        int x = start.x;
        int y = start.y;

        int underline_digit_spacing = 1;
        int width = fw - 1;

        display.draw_line({x + 1, y + fh + underline_digit_spacing},
                          {x + width, y + fh + underline_digit_spacing}, color);
}

void erase_digit_underline(const Display &display,
                           const UserInterfaceCustomization &customization,
                           const SquareCellGridDimensions &dimensions,
                           Point location)
{
        auto font_dimensions = display.get_font_configuration().font_dimensions;
        auto [fw, fh] = font_dimensions;
        Point start =
            calculate_cell_text_start(font_dimensions, dimensions, location);
        int x = start.x;
        int y = start.y;
        int underline_digit_spacing = 1;
        int underline_thickness = 1;
        display.clear_region(
            {x, y + fh},
            {x + fw, y + fh + underline_digit_spacing + underline_thickness},
            Black);
}

void erase_digit(const Display &display,
                 const UserInterfaceCustomization &customization,
                 const SquareCellGridDimensions &dimensions, Point location)
{
        auto font_dimensions = display.get_font_configuration().font_dimensions;
        auto [fw, fh] = display.get_font_configuration().font_dimensions;
        Point start =
            calculate_cell_text_start(font_dimensions, dimensions, location);
        int x = start.x;
        int y = start.y;

        display.clear_region({x, y}, {x + fw, y + fh}, Black);
}

void SimpleSudokuView::render_cell(const SudokuCell &cell,
                                   const Point &location)
{
        render_digit(*display, customization, dimensions, location, cell);
}
void SimpleSudokuView::underline_cell(const Point &location)
{
        render_digit_underline(*display, customization, dimensions, location);
}
void SimpleSudokuView::underline_cell(const Point &location, Color color)
{
        render_digit_underline(*display, color, dimensions, location);
}
void SimpleSudokuView::erase_cell_contents(const Point &location)
{
        erase_digit(*display, customization, dimensions, location);
        erase_digit_underline(*display, customization, dimensions, location);
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
 * Renders a small dot next to the currently selected digit in the available
 * digits selector on the left.
 */
void render_digit_selection_indicator(Display *display,
                                      SquareCellGridDimensions *dimensions,
                                      int selected_number, Color color)
{
        int x_margin = dimensions->left_horizontal_margin;
        int y_margin = dimensions->top_vertical_margin;

        int cell_size = dimensions->actual_height / 9;
        // We shift the x position of the indicator slightly to the right to
        // ensure that rerendering of the selector digits doesn't clip the
        // indicator.
        int x_offset_adjustment = 3;
        int x_offset = -1.5 * cell_size + x_offset_adjustment;
        int y_offset = (selected_number - 1) * cell_size;
        int circle_radius = 3;

        int padding = (cell_size - 2 * circle_radius) / 2;

        int x = x_margin + x_offset + padding;
        int y = y_margin + y_offset + padding;

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
                render_digit(*display, customization, dimensions, {-2, i},
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
        erase_digit(*display, customization, dimensions, location);
        render_digit(*display, customization, dimensions, location, cell,
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
        erase_digit(*display, customization, dimensions, location);
        render_digit(*display, customization, dimensions, location, cell);
}
