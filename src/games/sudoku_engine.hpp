#pragma once
#include <optional>
#include <cassert>
#include <vector>
#include "../common/point.hpp"


/**
 * Represents a single cell in a 9x9 Sudoku grid. It can be empty or hold a
 * value from 1 to 9. The `is_user_defined` flag is used to differentiate
 * between cells that are a part of the initial Sudoku grid and cannot be
 * modified or erased by the user.
 */
class SudokuCell
{
      public:
        std::optional<int> value;
        bool is_user_defined = true;

        SudokuCell(std::optional<int> value, bool is_user_defined)
            : value(value), is_user_defined(is_user_defined)
        {
                assert(1 <= value && value <= 9 &&
                       "Sudoku cells can only store values between 1 and "
                       "9 (inclusive).");
        };

        SudokuCell() = default;
};

using SudokuGrid = std::vector<std::vector<SudokuCell>>;

std::optional<Point> find_empty_cell(const SudokuGrid &grid);
std::vector<int> find_valid_numbers(SudokuGrid &grid, Point location);
