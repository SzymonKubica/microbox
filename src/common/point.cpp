#include "point.hpp"
#include "stdlib.h"
#include "platform/interface/input.hpp"

#include "maths_utils.hpp"

void translate(Point *p, Direction dir)
{
        switch (dir) {
        case Direction::UP:
                p->y -= 1;
                break;
        case Direction::DOWN:
                p->y += 1;
                break;
        case Direction::LEFT:
                p->x -= 1;
                break;
        case Direction::RIGHT:
                p->x += 1;
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

Point translate_pure(const Point &p, Direction dir) {
        switch (dir) {
        case Direction::UP:
                return {p.x, p.y - 1};
        case Direction::DOWN:
                return {p.x, p.y + 1};
        case Direction::LEFT:
                return {p.x - 1, p.y};
        case Direction::RIGHT:
                return {p.x + 1, p.y};
        default:
                // No translation for unknown direction
                return p;
        }
}

void translate_within_bounds(Point *p, Direction dir, int rows, int cols)
{
        switch (dir) {
        case Direction::UP:
                if (p->y > 0)
                        p->y -= 1;
                break;
        case Direction::DOWN:
                if (p->y < rows - 1)
                        p->y += 1;
                break;
        case Direction::LEFT:
                if (p->x > 0)
                        p->x -= 1;
                break;
        case Direction::RIGHT:
                if (p->x < cols - 1)
                        p->x += 1;
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

void translate_toroidal_array(Point *p, Direction dir, int rows, int cols)
{
        switch (dir) {
        case Direction::UP:
                p->y = mathematical_modulo(p->y - 1, rows);
                break;
        case Direction::DOWN:
                p->y = mathematical_modulo(p->y + 1, rows);
                break;
        case Direction::LEFT:
                p->x = mathematical_modulo(p->x - 1, cols);
                break;
        case Direction::RIGHT:
                p->x = mathematical_modulo(p->x + 1, cols);
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

std::vector<Point> get_neighbours_inside_grid(Point *point, int rows, int cols)
{
        std::vector<Point> neighbours;
        // Dereference for readability;
        Point p = *point;

        // We add adjacent neighbours if within grid
        if (p.y > 0)
                neighbours.push_back({.x = p.x, .y = p.y - 1});
        if (p.y < rows - 1)
                neighbours.push_back({.x = p.x, .y = p.y + 1});
        if (p.x > 0)
                neighbours.push_back({.x = p.x - 1, .y = p.y});
        if (p.x < cols - 1)
                neighbours.push_back({.x = p.x + 1, .y = p.y});

        // We add diagonal neighbours if within grid
        if (p.y > 0 && p.x > 0)
                neighbours.push_back({.x = p.x - 1, .y = p.y - 1});
        if (p.y < rows - 1 && p.x < cols - 1)
                neighbours.push_back({.x = p.x + 1, .y = p.y + 1});
        if (p.x > 0 && p.y < rows - 1)
                neighbours.push_back({.x = p.x - 1, .y = p.y + 1});
        if (p.x < cols - 1 && p.y > 0)
                neighbours.push_back({.x = p.x + 1, .y = p.y - 1});

        return neighbours;
}

std::vector<Point> get_neighbours_toroidal_array(Point *point, int rows,
                                                 int cols)
{
        std::vector<Point> neighbours;
        // Dereference for readability;
        Point p = *point;

        neighbours.push_back(
            {.x = p.x, .y = mathematical_modulo(p.y - 1, rows)});
        neighbours.push_back(
            {.x = p.x, .y = mathematical_modulo(p.y + 1, rows)});
        neighbours.push_back(
            {.x = mathematical_modulo(p.x - 1, cols), .y = p.y});
        neighbours.push_back(
            {.x = mathematical_modulo(p.x + 1, cols), .y = p.y});
        neighbours.push_back({.x = mathematical_modulo(p.x - 1, cols),
                              .y = mathematical_modulo(p.y - 1, rows)});
        neighbours.push_back({.x = mathematical_modulo(p.x + 1, cols),
                              .y = mathematical_modulo(p.y + 1, rows)});
        neighbours.push_back({.x = mathematical_modulo(p.x - 1, cols),
                              .y = mathematical_modulo(p.y + 1, rows)});
        neighbours.push_back({.x = mathematical_modulo(p.x + 1, cols),
                              .y = mathematical_modulo(p.y - 1, rows)});
        return neighbours;
}

bool is_adjacent(Point *p1, Point *p2)
{
        return (abs(p1->x - p2->x) <= 1 && abs(p1->y - p2->y) <= 1);
}
