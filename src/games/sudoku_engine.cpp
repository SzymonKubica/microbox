#include <algorithm>
#include <memory>
#include "sudoku_engine.hpp"
#include "../common/logging.hpp"
#include "../common/point.hpp"

#define TAG "sudoku_engine"

/*
 * Note: by convention in this file we are not using any constants for the
 * int literal '9'. This is because it is a real 'magic number' in the sudoku
 * definition. There are only 9 digits in Sudoku and it will never change.
 * Having a `#define` constant to hide this resulted in the code being a bit
 * clunky and not explicit enough. Because of this you will find raw '9' digits
 * sprinkled in the implementation below.
 */

/* Forward-declarations of the utility functions used by the Sudoku solver */
std::optional<Point> find_empty_cell(const SudokuGrid &grid);
std::vector<int> find_valid_numbers(const SudokuGrid &grid, Point location);

/**
 * Recursive algorithm that solves the supplied Sudoku grid by modifying it in
 * place.
 *
 * It uses a recursive backtracking approach: for each empty cell try to place
 * each of the 'valid' numbers (according to the usual sudoku constraints) and
 * if the given placement doesn't lead to a solvable grid, try again with the
 * next number.
 */
bool SudokuEngine::solve(SudokuGrid &grid)
{
        std::optional<Point> maybe_location = find_empty_cell(grid);
        if (!maybe_location.has_value())
                return true;
        Point empty = maybe_location.value();
        LOG_DEBUG(TAG, "Found empty cell: {x: %d, y: %d}", empty.x, empty.y);

        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);
        LOG_DEBUG(TAG, "Found valid %d numbers", valid_numbers.size());

        auto &empty_cell = grid[empty.y][empty.x];
        for (int candidate : valid_numbers) {
                empty_cell.digit = candidate;
                if (solve(grid)) {
                        return true;
                }
                // If the candidate value didn't lead us to the correct
                // solution, we roll it back and try the next candidate.
                empty_cell.digit = std::nullopt;
        }
        return false;
}

/**
 * Returns coordinates of an empty cell in a Sudoku grid (if one exists).
 */
std::optional<Point> find_empty_cell(const SudokuGrid &grid)
{
        int x, y;
        y = 0;
        for (auto &row : grid) {
                x = 0;
                for (auto &cell : row) {
                        if (cell.is_user_defined && !cell.digit.has_value()) {
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
std::vector<int> find_valid_numbers(const SudokuGrid &grid, Point location)
{
        auto &cell_at_location = grid[location.y][location.x];

        assert(!cell_at_location.digit.has_value() &&
               "Cell tested for candidate numbers has to be empty.");

        std::vector<int> candidates = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto remove_candidate_digit = [&](int digit) {
                auto idx =
                    std::find(candidates.begin(), candidates.end(), digit);
                if (idx != candidates.end())
                        candidates.erase(idx);
        };

        auto remove_candidate_if_cell_has_value = [&](const SudokuCell &cell) {
                if (cell.digit.has_value())
                        remove_candidate_digit(cell.digit.value());
        };

        // Check the row
        for (int x = 0; x < 9; x++) {
                const auto &cell = grid[location.y][x];
                remove_candidate_if_cell_has_value(cell);
        }

        // Check the column
        for (int y = 0; y < 9; y++) {
                const auto &cell = grid[y][location.x];
                remove_candidate_if_cell_has_value(cell);
        }

        // Check the big square
        // We round down the coordinates of the selected location to the
        // multiple of 3 to find the top left corner of the square.
        Point square_start = {3 * (location.x / 3), 3 * (location.y / 3)};
        for (int y = square_start.y; y < square_start.y + 3; y++) {
                for (int x = square_start.x; x < square_start.x + 3; x++) {
                        const auto &current = grid[y][x];
                        remove_candidate_if_cell_has_value(current);
                }
        }
        return candidates;
}

/*
 * Forward-declarations of utility functions used when generating a solveable
 * sudoku grid.
 */
SudokuGrid generate_solved_grid();
bool populate_solved_grid(SudokuGrid &grid);

/**
 * Given a difficulty level between 1 and 3 (inclusive) it generates a sudoku
 * grid that has a unique solution.
 *
 * Note that this is potentially expensive as it involves repeatedly trying to
 * remove random cells and checking with the solver if the resulting grid still
 * has a unique solution.
 */
SudokuGrid SudokuEngine::generate_grid(int difficulty_level)
{
        assert(1 <= difficulty_level && difficulty_level <= 3 &&
               "Difficulty level has to be between 1 and 3 (inclusive).");
        auto solvable = generate_solved_grid();

        // According to the online wisdom, a sudoku with ~40 cells left
        // tends to be easy. If you leave ~30 it becomes medium and if only
        // ~20 cells are left it turns out to be hard. Because of this, we
        // calculate the number of digits to remove as:
        // 30 + 10 * x where x is the difficulty level (1, 2, or 3).
        // This gives us: 1 -> 40, 2 -> 50 and 3 -> 60 which is what we want.
        int to_remove = 30 + 10 * difficulty_level;

        std::vector<Point> locations_to_remove;

        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        locations_to_remove.push_back({x, y});
                }
        }

        // We shuffle the candidate numbers positions to ensure that the
        // generated empty number pattern is random.
        std::shuffle(locations_to_remove.begin(), locations_to_remove.end(),
                     RandomGenerator());

        int candidate_idx = 0;
        int removed = 0;

        while (removed < to_remove &&
               candidate_idx < locations_to_remove.size()) {
                LOG_DEBUG(
                    TAG,
                    "Trying to remove candidate cell %d (%d removed so far)",
                    candidate_idx, removed);
                Point loc = locations_to_remove[candidate_idx];
                int x = loc.x;
                int y = loc.y;

                auto &cell = solvable[y][x];
                int previous_value = cell.digit.value();
                cell.digit = std::nullopt;
                cell.is_user_defined = true;

#if defined(EMULATOR) || defined(ARDUINO_ARCH_ESP32)
                if (SudokuEngine::has_unique_solution(solvable)) {
#else
                  //TODO: optimize the uniqueness checking logic to work on arduino
                if (true) {
#endif
                        removed++;
                } else {
                        cell.digit = previous_value;
                        cell.is_user_defined = false;
                }

                candidate_idx++;
        }

        // After the grid is generated, we need to make all cells
        // non-user-defined.
        for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 9; x++) {
                        if (solvable[y][x].digit.has_value()) {
                                solvable[y][x].is_user_defined = false;
                        }
                }
        }

        return solvable;
}

/*
 * Given a grid that is potentially empty, it populates it with an arrangement
 * of numbers that constitutes a solved sudoku grid. It is exactly the same as
 * the solve function, but it shuffles the list of available valid numbers to
 * ensure that the generated grid is random. When solving the sudoku 'for real'
 * we don't want to shuffle as it is potentially expensive.
 */
bool populate_solved_grid(std::vector<std::vector<SudokuCell>> &grid)
{
        std::optional<Point> maybe_location = find_empty_cell(grid);
        if (!maybe_location.has_value())
                return true;
        Point empty = maybe_location.value();
        LOG_DEBUG(TAG, "Found empty cell: {x: %d, y: %d}", empty.x, empty.y);

        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);
        LOG_DEBUG(TAG, "Found valid %d numbers", valid_numbers.size());

        std::shuffle(valid_numbers.begin(), valid_numbers.end(),
                     RandomGenerator{});

        auto &empty_cell = grid[empty.y][empty.x];
        for (int candidate : valid_numbers) {
                empty_cell.digit = candidate;
                if (populate_solved_grid(grid)) {
                        return true;
                }
                empty_cell.digit = std::nullopt;
        }

        return false;
}

SudokuGrid generate_solved_grid()
{
        std::vector<std::vector<SudokuCell>> grid(
            9, std::vector(9, SudokuCell(std::nullopt, true)));

        populate_solved_grid(grid);
        return grid;
}

bool test_for_unique_solution(SudokuGrid &grid, int *solution_count);

/**
 * Given a Sudoku grid, it verifies if it can be solved and the solution
 * is unique.
 *
 * Note that this copies the supplied grid and runs the solver algorithm
 * on the grid multiple times. Hence it can get expensive and should be used
 * with caution.
 */
bool SudokuEngine::has_unique_solution(const SudokuGrid &grid)
{

        SudokuGrid clone = grid;
        int solution_count = 0;
        test_for_unique_solution(clone, &solution_count);
        return solution_count == 1;
}

/**
 * Tests if a given grid can be uniquely solved
 */
bool test_for_unique_solution(SudokuGrid &grid, int *solution_count)
{

        // Prune the search space to return true immediately once more than 1
        // solution is found.
        if (*solution_count > 1) {
                return true;
        }

        std::optional<Point> maybe_empty = find_empty_cell(grid);
        if (!maybe_empty.has_value()) {
                (*solution_count)++;
                return true;
        }

        Point empty = maybe_empty.value();
        std::vector<int> valid_numbers = find_valid_numbers(grid, empty);

        for (int candidate : valid_numbers) {
                grid[empty.y][empty.x].digit = candidate;
                // Here we don't skip if a solution is found. Instead we try
                // all candidates and rely on the short-circuit logic above
                // to terminate once more than on solution is found
                test_for_unique_solution(grid, solution_count);
                if (*solution_count > 1) {
                        return true;
                }
                grid[empty.y][empty.x].digit = std::nullopt;
        }

        return false;
}

/**
 * Given an immutable sudoku grid, it verifies if it contains a valid solution
 * to the puzzle.
 */
#define NINE_ZEROS {0, 0, 0, 0, 0, 0, 0, 0, 0}
bool SudokuEngine::validate(const std::vector<std::vector<SudokuCell>> &grid)
{

        // Check rows
        for (int y = 0; y < 9; y++) {
                LOG_DEBUG(TAG, "Validating row %d", y);
                // On Arduino we need to explicity allocate the array with
                // zeros, else it will contain random garbage values.
                int digit_counts[9] = NINE_ZEROS;
                for (int x = 0; x < 9; x++) {
                        auto &cell = grid[y][x];
                        if (!cell.digit.has_value())
                                return false;
                        int digit = cell.digit.value();
                        int digit_idx = digit - 1;
                        digit_counts[digit_idx]++;
                        int count = digit_counts[digit_idx];
                        if (count != 1) {
                                LOG_DEBUG(TAG, "%d found %d times. Incorrect.",
                                          digit, count);
                                return false;
                        }
                }
        }
        LOG_DEBUG(TAG, "Rows validated successfully!");

        // Check columns
        for (int x = 0; x < 9; x++) {
                LOG_DEBUG(TAG, "Validating column %d", x);
                int digit_counts[9] = NINE_ZEROS;
                for (int y = 0; y < 9; y++) {
                        auto &cell = grid[y][x];
                        if (!cell.digit.has_value())
                                return false;
                        int digit = cell.digit.value();
                        int digit_idx = digit - 1;
                        digit_counts[digit_idx]++;
                        int count = digit_counts[digit_idx];
                        if (count != 1) {
                                LOG_DEBUG(TAG, "%d found %d times. Incorrect.",
                                          digit, count);
                                return false;
                        }
                }
        }
        LOG_DEBUG(TAG, "Columns validated successfully!");

        for (int y = 0; y < 9; y += 3) {
                for (int x = 0; x < 9; x += 3) {
                        // Check big square
                        Point square_top_left = {x, y};
                        LOG_DEBUG(TAG, "Validating square starting at (%d, %d)",
                                  x, y);
                        int digit_counts[9] = NINE_ZEROS;
                        for (int y = square_top_left.y;
                             y < square_top_left.y + 3; y++) {
                                for (int x = square_top_left.x;
                                     x < square_top_left.x + 3; x++) {

                                        auto &cell = grid[y][x];
                                        if (!cell.digit.has_value())
                                                return false;
                                        int digit = cell.digit.value();
                                        int digit_idx = digit - 1;
                                        digit_counts[digit_idx]++;
                                        int count = digit_counts[digit_idx];
                                        if (count != 1) {
                                                LOG_DEBUG(TAG,
                                                          "%d found %d times. "
                                                          "Incorrect.",
                                                          digit, count);
                                                return false;
                                        }
                                }
                        }
                }
        }
        LOG_DEBUG(TAG, "Squares validated successfully!");
        return true;
}
