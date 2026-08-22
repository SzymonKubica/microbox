#include "point.hpp"
#include "stdlib.h"
#include "../platform/interface/input.hpp"

#include "maths_utils.hpp"
#include <optional>

IntPoint operator+(IntPoint first, IntPoint second)
{
        return {first.x + second.x, first.y + second.y};
}

IntPoint IntPoint::operator*(int scalar) const
{
        return {scalar * x, scalar * y};
}

bool operator==(const IntPoint &first, const IntPoint &second)
{
        return first.x == second.x && first.y == second.y;
}

void translate(IntPoint &p, Direction dir)
{
        switch (dir) {
        case Direction::UP:
                p.y -= 1;
                break;
        case Direction::DOWN:
                p.y += 1;
                break;
        case Direction::LEFT:
                p.x -= 1;
                break;
        case Direction::RIGHT:
                p.x += 1;
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

IntPoint translate_pure(const IntPoint &p, Direction dir)
{
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

std::optional<Direction> determine_displacement_direction(IntPoint &reference,
                                                          IntPoint &target)
{

        bool same_column = reference.x == target.x;
        bool same_row = reference.y == target.y;
        if (same_column) {
                if (reference.y - 1 == target.y)
                        return Direction::UP;
                if (reference.y + 1 == target.y)
                        return Direction::DOWN;
        }
        if (same_row) {
                if (reference.x + 1 == target.x)
                        return Direction::RIGHT;
                if (reference.x - 1 == target.x)
                        return Direction::LEFT;
        }
        return std::nullopt;
}

void translate_within_bounds(IntPoint &p, Direction dir, int rows, int cols)
{
        switch (dir) {
        case Direction::UP:
                if (p.y > 0)
                        p.y -= 1;
                break;
        case Direction::DOWN:
                if (p.y < rows - 1)
                        p.y += 1;
                break;
        case Direction::LEFT:
                if (p.x > 0)
                        p.x -= 1;
                break;
        case Direction::RIGHT:
                if (p.x < cols - 1)
                        p.x += 1;
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

void translate_toroidal_array(IntPoint &p, Direction dir, int rows, int cols)
{
        switch (dir) {
        case Direction::UP:
                p.y = mathematical_modulo(p.y - 1, rows);
                break;
        case Direction::DOWN:
                p.y = mathematical_modulo(p.y + 1, rows);
                break;
        case Direction::LEFT:
                p.x = mathematical_modulo(p.x - 1, cols);
                break;
        case Direction::RIGHT:
                p.x = mathematical_modulo(p.x + 1, cols);
                break;
        default:
                // No translation for unknown direction
                break;
        }
}

std::vector<IntPoint> get_neighbours_inside_grid(const IntPoint &point,
                                                 int rows, int cols)
{
        std::vector<IntPoint> neighbours;
        // alias for readability;
        auto &p = point;

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

std::vector<IntPoint> get_adjacent_neighbours_inside_grid(const IntPoint &point,
                                                          int rows, int cols)
{
        std::vector<IntPoint> neighbours;
        // alias for readability;
        IntPoint p = point;

        // We add adjacent neighbours if within grid
        if (p.y > 0)
                neighbours.push_back({.x = p.x, .y = p.y - 1});
        if (p.y < rows - 1)
                neighbours.push_back({.x = p.x, .y = p.y + 1});
        if (p.x > 0)
                neighbours.push_back({.x = p.x - 1, .y = p.y});
        if (p.x < cols - 1)
                neighbours.push_back({.x = p.x + 1, .y = p.y});

        return neighbours;
}

std::vector<IntPoint> get_neighbours_toroidal_array(const IntPoint &point,
                                                    int rows, int cols)
{
        std::vector<IntPoint> neighbours;
        // alias for readability;
        IntPoint p = point;

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

bool is_adjacent(const IntPoint &p1, const IntPoint &p2)
{
        return (abs(p1.x - p2.x) <= 1 && abs(p1.y - p2.y) <= 1);
}

/**
 * Scalar product
 */
Point Point::operator*(double scalar) { return {scalar * x, scalar * y}; }
/**
 * Dot product
 */
double Point::operator*(Point other) { return x * other.x + y * other.y; }

IntPoint Point::cast() { return IntPoint{(int)x, (int)y}; }

Point operator+(const Point &p1, const Point &p2)
{
        return {p1.x + p2.x, p1.y + p2.y};
}
Point operator-(const Point &p1, const Point &p2)
{
        return {p1.x - p2.x, p1.y - p2.y};
}
