#pragma once
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <cassert>
#include <vector>

/**
 * Represents a single cell in a 9x9 Sudoku grid. It can be empty or hold a
 * value from 1 to 9. The `is_user_defined` flag is used to differentiate
 * between cells that are a part of the initial Sudoku grid and cannot be
 * modified or erased by the user.
 */
class SudokuCell
{
      public:
        std::optional<int> digit;
        bool is_user_defined = true;

        SudokuCell(std::optional<int> digit, bool is_user_defined)
            : digit(digit), is_user_defined(is_user_defined) {
              //TODO: fix this
                      /*
                            assert(1 <= digit && digit <= 9 &&
                                   "Sudoku cells can only store values between 1
                         and " "9 (inclusive).");
                                   */
              };

        SudokuCell() = default;
};

/**
 * Implements uniform random number generator (URBG) 'interface'.
 *
 * This is required to be able to `std::shuffle` a vector by sloting the
 * default `rand()` generator into the URBG interface required by the shuffle
 * function.
 */
class RandomGenerator
{
      public:
        using result_type = uint32_t;
        result_type operator()() { return rand(); }
        static constexpr result_type min() { return 0; }
        static constexpr result_type max()
        {
                return static_cast<uint32_t>(RAND_MAX);
        }
};

using SudokuGrid = std::vector<std::vector<SudokuCell>>;

namespace SudokuEngine
{
bool solve(SudokuGrid &grid);
bool has_unique_solution(const SudokuGrid &grid);
bool validate(const SudokuGrid &grid);
SudokuGrid generate_grid(int difficulty_level);
} // namespace SudokuEngine
