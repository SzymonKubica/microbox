#include <algorithm>
#include "sudoku_engine.hpp"
#include "../common/logging.hpp"

#define TAG "sudoku_engine"

/**
 * Recursive algorithm that solves the supplied Sudoku grid by modifying it in
 * place.
 */
bool solve(SudokuGrid &grid)
{
        std::optional<Point> maybe_location = find_empty_cell(grid);
        if (!maybe_location.has_value())
                return true;
        Point empty = maybe_location.value();
        LOG_DEBUG(TAG, "Found empty cell: {x: %d, y: %d}", empty.x, empty.y);

        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);
        LOG_DEBUG(TAG, "Found valid %d numbers", valid_numbers.size());

        for (int candidate : valid_numbers) {
                auto &cell = grid[empty.y][empty.x];
                cell.value = candidate;
                if (solve(grid)) {
                        return true;
                }
                // If the candidate value didn't lead us to the correct
                // solution, we roll it back and try the next candidate.
                cell.value = std::nullopt;
        }
        return false;
}

/**
 * Returns coordinates of an empty cell in a Sudoku grid (if exists).
 */
std::optional<Point> find_empty_cell(const SudokuGrid &grid)
{
        int x, y;
        y = 0;
        for (auto &row : grid) {
                x = 0;
                for (auto &cell : row) {
                        if (cell.is_user_defined && !cell.value.has_value()) {
                                Point location = {x, y};
                                return location;
                        }
                        x++;
                }
                y++;
        }
        return std::nullopt;
}

/**
 * Applies sudoku rules to determine the set of candidate numbers that can be
 * placed on the `location` in the grid. Note that in order to do this the
 * tested location has to be empty.
 */
std::vector<int> find_valid_numbers(SudokuGrid &grid, Point location)
{
        auto &cell_at_location = grid[location.y][location.x];

        assert(!cell_at_location.value.has_value() &&
               "Cell tested for candidate numbers has to be empty.");

        std::vector<int> candidates = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto remove_candidate = [&](int candidate_digit) {
                auto position = std::find(candidates.begin(), candidates.end(),
                                          candidate_digit);
                if (position != candidates.end()) {
                        candidates.erase(position);
                }
        };

        // Check the row
        for (int x = 0; x < 9; x++) {
                auto &cell = grid[location.y][x];
                if (cell.value.has_value())
                        remove_candidate(cell.value.value());
        }

        // Check the column
        for (int y = 0; y < 9; y++) {
                auto &cell = grid[y][location.x];
                if (cell.value.has_value())
                        remove_candidate(cell.value.value());
        }

        // Check the big square
        // We round down the coordinates of the selected location to the
        // multiple of 3 to find the top left corner of the square.
        Point square_start = {3 * (location.x / 3), 3 * (location.y / 3)};
        for (int y = square_start.y; y < square_start.y + 3; y++) {
                for (int x = square_start.x; x < square_start.x + 3; x++) {
                        auto &current = grid[y][x];
                        if (current.value.has_value())
                                remove_candidate(current.value.value());
                }
        }
        return candidates;
}
