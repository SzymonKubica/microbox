#pragma once
#include "../common/grid.hpp"

namespace SnakeDefinitions
{
enum class Cell : uint8_t {
        Empty,
        Snake,
        Apple,
        // After an apple is eaten this is left on the grid. Note that from
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Empty`
        Poop,
        // Corresponds to a segment of snake's body that has an apple
        // inside of it (this happens after the snake eats an apple). From the
        // game mechanics perspective this is equivalent to
        // `SnakeGridCell::Snake` but it gets rendered differently.
        AppleSnake,
};

struct Snake {
        Point head;
        Point tail;
        Direction direction;
        std::vector<Point> body;

      public:
        Snake(Point head, Direction direction);

        /**
         * Moves the snake one unit along its current direction.
         */
        void take_step();

        /**
         * Returns the location of the segment right behind the head.
         */
        Point get_neck();
};

/**
 * Spawns an apple at a random empty location on the grid. Note that if there
 * are no empty locations left on the grid, this function will spin forever.
 */
Point spawn_apple(std::vector<std::vector<Cell>> *grid);
/**
 * (re-)renders a single cell on the grid based on its current value.
 * Note that this needs to be called each time the value of a cell on the grid
 * is changed. We cannot redraw the entire display as the WaveShare LCD we
 * are using is too slow (draws one pixel at a time).
 */
void refresh_grid_cell(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, Point &location);
/**
 * Renders a segment that connects two adjacent snake segments on the grid.
 */
void render_segment_connection(Display *display, Color snake_color,
                               SquareCellGridDimensions *dimensions,
                               std::vector<std::vector<Cell>> *grid,
                               Point &first_location, Point &second_location);
/**
 * Renders a segment that connects two adjacent snake segments on the grid.
 */
void render_snake_head(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid,
                       const Snake &snake);

} // namespace SnakeDefinitions
