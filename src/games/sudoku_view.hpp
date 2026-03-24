#include "../common/point.hpp"
#include "../common/user_interface_customization.hpp"
#include "../common/grid.hpp"
#include "../common/platform/interface/color.hpp"
#include "sudoku_engine.hpp"

class SudokuView
{
      public:
        /* Grid Cell Rendering */
        /**
         * Renders the sudoku grid frame.
         */
        virtual void render_grid() = 0;
        /**
         * Renders all numbers that are present on the grid.
         */
        virtual void render_grid_numbers(const SudokuGrid &grid) = 0;
        /**
         * Given a cell on the sudoku grid and a target location on the grid,
         * it renders the cell value there.
         */
        virtual void render_cell(const SudokuCell &cell,
                                 const Point &location) = 0;
        /**
         * Underlines a digit placed in a cell under a given location.
         */
        virtual void underline_cell(const Point &location) = 0;
        virtual void underline_cell(const Point &location, Color color) = 0;
        /**
         * Erases the digit that was placed on a given cell.
         */
        virtual void erase_cell_contents(const Point &location) = 0;
        /**
         * Underlines all occurrences of the given digit with the specified
         * color. This is a good visual indicator that helps with readability
         * and makes solving the sudoku easier.
         */
        virtual void underline_all_instances(int digit,
                                             const SudokuGrid &grid) = 0;
        virtual void underline_all_instances(int digit, const SudokuGrid &grid,
                                             Color color) = 0;
        /**
         * When the active digit changes we need to remove the underline that
         * was placed below all occurrences of the previously active digit.
         * This function achieves that.
         */
        virtual void remove_underline_all_instances(int digit,
                                                    const SudokuGrid &grid) = 0;

        /* User-controlled Caret Rendering */
        /**
         * Renders the caret at a given grid location.
         */
        virtual void render_caret(const Point &location) = 0;
        /**
         * Erases the caret at a given grid location.
         */
        virtual void erase_caret(const Point &location) = 0;
        /**
         * Moves the caret from a given location on the Sudoku grid to the
         * destination location.
         */
        virtual void move_caret(const Point &from, const Point &to) = 0;
        /**
         * Re-renders the caret at a given location. This is sometimes required
         * if updates to cells 'clip' the caret.
         */
        virtual void rerender_caret(const Point &location) = 0;

        /* Active Digit Selector */
        /**
         * When the user changes the active digit, we need to update the
         * available digits selector to move the selection indicator to the new
         * active digit.
         */
        virtual void
        move_active_digit_selection_indicator(int previous_digit,
                                              int current_active_digit) = 0;
        /**
         * Used when rendering the active digit selector for the first time.
         */
        virtual void
        render_active_digit_selection_indicator(int active_digit) = 0;
        /**
         * Renders the list of available digits that the users will use to
         * select the active number to place on the grid.
         */
        virtual void render_active_digit_selector() = 0;
        /**
         * Once the user places 9 occurrences of a given digit on the grid,
         * we mark it as 'completed' and highlight it in the active digit
         * selector. This improves clarity and makes it easier to see which
         * numbers are done.
         */
        virtual void mark_digit_completed(int digit) = 0;
        /**
         * If a user placed 9 occurrences of a given digit on the grid but then
         * realized that this was wrong and erased some of them, we need to
         * rollback this 'completed' indicator.
         */
        virtual void unmark_digit_completed(int digit) = 0;
};

/**
 * Default implementation of the `SudokuView` interface.
 */
class SimpleSudokuView : SudokuView
{
        UserInterfaceCustomization customization;
        SquareCellGridDimensions dimensions;
        Display *display;

      public:
        SimpleSudokuView(UserInterfaceCustomization customization,
                         SquareCellGridDimensions dimensions, Display *display)
            : customization(customization), dimensions(dimensions),
              display(display)
        {
        }
        void render_grid() override;
        void render_grid_numbers(const SudokuGrid &grid) override;
        void underline_all_instances(int digit,
                                     const SudokuGrid &grid) override;
        void underline_all_instances(int digit, const SudokuGrid &grid,
                                     Color color) override;
        void remove_underline_all_instances(int digit,
                                            const SudokuGrid &grid) override;
        void render_cell(const SudokuCell &cell,
                         const Point &location) override;
        void underline_cell(const Point &location) override;
        void underline_cell(const Point &location, Color color) override;
        void erase_cell_contents(const Point &location) override;

        void move_caret(const Point &from, const Point &to) override;
        void render_caret(const Point &location) override;
        void erase_caret(const Point &location) override;
        void rerender_caret(const Point &location) override;

        void move_active_digit_selection_indicator(
            int previous_digit, int current_active_digit) override;
        void render_active_digit_selection_indicator(int active_digit) override;
        void render_active_digit_selector() override;
        void mark_digit_completed(int digit) override;
        void unmark_digit_completed(int digit) override;
};
