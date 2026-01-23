#include "snake_common.hpp"
#include "../common/logging.hpp"

#define TAG "snake_common"

namespace SnakeDefinitions
{
Snake::Snake(Point head, Direction direction) : head(head), direction(direction)
{
        Point tail = head;
        translate(&tail, get_opposite(direction));
        this->tail = tail;
        body = {this->tail, head};
}

void Snake::take_step() { translate(&this->head, this->direction); }

Point Snake::get_neck() { return *(this->body.end() - 2).base(); }

/**
 * Renders the head of the snake. This contains snake's eye and a rounded front
 * part of the snake.
 */
void render_snake_head(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, const Snake &snake)
{

        // Calculation logic to map from logical cells to the actual pixel
        // values. Note that this is a duplicate of the same logic in
        // `re_render_grid_cell` and could be refactored into some other
        // function or merged into grid dimensions struct if it becomes useful
        // somewhere else.
        int padding = 2;
        // rectangle border width needs to be non-zero, else the physical LCD
        // display will not render rectangles.
        int border_width = 1;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        // We start drawing from the end of the padded square, hence we
        // add `width - padding` below to get to that point. Also note
        // that we need to start drawing from the padded vertical start
        // hence we add the padding in the y coordinate
        Point start = {.x = left_margin + snake.head.x * width,
                       .y = top_margin + snake.head.y * height};

        // We draw a 'half-cell' to connect snake head to the neck.
        int snake_w = width - 2 * padding;
        int snake_h = height - 2 * padding;
        int rectangle_w, rectangle_h;

        switch (snake.direction) {
        case UP:
        case DOWN:
                rectangle_w = snake_w;
                rectangle_h = snake_h / 2;
                break;
        case RIGHT:
        case LEFT:
                rectangle_w = snake_w / 2;
                rectangle_h = snake_h;
        }

        Point offset, eye_offset;
        // When going down we need to make the rectangle a bit larger.
        // TODO: clean up
        int vertical_extension = 0;
        int height_adj = 0;
        switch (snake.direction) {
        case UP:
                offset = {.x = 0, .y = snake_h / 2};
                eye_offset = {.x = -rectangle_w / 4, .y = 0};
                height_adj = 1;
                break;
        case LEFT:
                offset = {.x = snake_w / 2, .y = 0};
                eye_offset = {.x = 0, .y = rectangle_h / 4};
                break;
        case DOWN:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = rectangle_w / 4, .y = 0};
                vertical_extension = 3;
                break;
        case RIGHT:
                offset = {.x = 0, .y = 0};
                eye_offset = {.x = 0, .y = -rectangle_h / 4};
                break;
        }

        display->draw_rectangle(
            {.x = start.x + offset.x + padding,
             .y = start.y + offset.y + padding - vertical_extension},
            rectangle_w, rectangle_h + vertical_extension + height_adj,
            snake_color, border_width, true);
        Point cell_center = {.x = start.x + width / 2,
                             .y = start.y + height / 2};
        Point eye_center = {
            .x = start.x + offset.x + padding + rectangle_w / 2 + eye_offset.x,
            .y = start.y + offset.y + padding + rectangle_h / 2 + eye_offset.y};
        display->draw_circle(cell_center, (width - 2 * padding) / 2,
                             snake_color, border_width, true);
        display->draw_circle(eye_center, 1, Black, 0, true);
}

/**
 * Renders a segment that connects two adjacent locations on the grid.
 * Note that this function assumes that the two points are adjacent and the
 * caller of the function needs to ensure that this is indeed the case.
 * Also note that here we are referring to the 'logical' locations, i.e. cells
 * on the grid, not the actual pixel locations. This function performs the
 * translation between the two concepts.
 */
void render_segment_connection(Display *display, Color snake_color,
                               SquareCellGridDimensions *dimensions,
                               std::vector<std::vector<Cell>> *grid,
                               Point &first_location, Point &second_location)
{

        // Calculation logic to map from logical cells to the actual pixel
        // values. Note that this is a duplicate of the same logic in
        // `re_render_grid_cell` and could be refactored into some other
        // function or merged into grid dimensions struct if it becomes useful
        // somewhere else.
        int padding = 2;
        // Rectangle border width needs to be non-zero, else the physical LCD
        // display will not render rectangles.
        int border_width = 1;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        bool adjacent_horizontally = first_location.y == second_location.y;

        LOG_DEBUG(TAG,
                  "Rendering segment connection between: {x: %d, y: %d} and "
                  "{x: %d, y: %d}",
                  first_location.x, first_location.y, second_location.x,
                  second_location.y);

        Point start;
        int segment_width;
        int segment_height;
        if (adjacent_horizontally) {
                Point &left_point = first_location.x < second_location.x
                                        ? first_location
                                        : second_location;

                // We start drawing from the end of the padded square, hence we
                // add `width - padding below to get to that point` also note
                // that we need to start drawing from the padded vertical start
                // hence we add the padding in the y coordinate
                start = {.x = left_margin + (left_point.x + 1) * width,
                         .y = top_margin + left_point.y * height + padding};
                segment_width = padding - border_width;
                segment_height = height - 2 * padding;

        } else {
                Point &top_point = first_location.y < second_location.y
                                       ? first_location
                                       : second_location;
                // We start drawing from the end of the padded square (bottom
                // left corner), hence we add `height - padding below to get to
                // that point`
                start = {.x = left_margin + top_point.x * width + padding,
                         .y = top_margin + (top_point.y + 1) * height};
                segment_width = width - 2 * padding;
                segment_height = padding - border_width;
        }

        display->draw_rectangle(start, segment_width, segment_height,
                                snake_color, border_width, true);
}

void refresh_grid_cell(Display *display, Color snake_color,
                       SquareCellGridDimensions *dimensions,
                       std::vector<std::vector<Cell>> *grid, Point &location)
{
        int padding = 2;
        int height = dimensions->actual_height / dimensions->rows;
        int width = dimensions->actual_width / dimensions->cols;
        int left_margin = dimensions->left_horizontal_margin;
        int top_margin = dimensions->top_vertical_margin;

        Point start = {.x = left_margin + location.x * width,
                       .y = top_margin + location.y * height};

        Cell cell_type = (*grid)[location.y][location.x];

        // When rendering on the actual lcd display the circle comes out a bit
        // larger because of pixel inaccuracies and internal of that lcd display
        // driver. Because of this we need to decrease the radius slightly.
        int radius_offset = 1;

        // Represents the top left corner of the snake segment
        Point padded_start = {.x = start.x + padding, .y = start.y + padding};
        Point apple_center = {.x = start.x + width / 2,
                              .y = start.y + width / 2};

        switch (cell_type) {
        case Cell::Apple: {
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Red, 0, true);
                break;
        }
        case Cell::Snake: {
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                break;
        }
        case Cell::Empty: {
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                break;
        }
        case Cell::AppleSnake: {
                // Here we render a normal snake segment that has a hole inside
                // of it with an apple sitting there. This is to indicate
                // segments of the snake that have 'consumed an apple'
                display->draw_rectangle(padded_start, width - 2 * padding,
                                        height - 2 * padding, snake_color, 1,
                                        true);
                display->draw_circle(apple_center,
                                     (width - 2 * padding) / 2 - radius_offset,
                                     Color::Black, 0, true);
                display->draw_circle(
                    apple_center, (width - 2 * padding) / 2 - 2 * radius_offset,
                    Color::Red, 0, true);
                break;
        }
        case Cell::Poop: {

                // First we clear the cell and then we draw a pile of shit.
                display->draw_rectangle(start, width, height, Color::Black, 1,
                                        true);
                auto locations = {
                    apple_center,
                    {.x = apple_center.x + 2, .y = apple_center.y},
                    {.x = apple_center.x + 1, .y = apple_center.y - 2}};
                for (Point loc : locations) {
                        display->draw_circle(
                            loc, (width - 2 * padding) / 2 - radius_offset,
                            Color::Brown, 0, true);
                }
                break;
        }
        }
}

Point spawn_apple(std::vector<std::vector<Cell>> *grid)
{
        int rows = grid->size();
        int cols = (*grid->begin().base()).size();
        while (true) {
                int x = rand() % cols;
                int y = rand() % rows;

                Cell selected = (*grid)[y][x];
                if (selected != Cell::Snake) {
                        (*grid)[y][x] = Cell::Apple;
                        return {x, y};
                        break;
                }
        }
}
} // namespace SnakeDefinitions
