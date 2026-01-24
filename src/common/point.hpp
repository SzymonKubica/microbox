#pragma once
#include "platform/interface/input.hpp"
#include <vector>
typedef struct Point {
        int x;
        int y;

} Point;

void translate(Point *p, Direction dir);
Point translate_pure(const Point &p, Direction dir);
void translate_within_bounds(Point *p, Direction dir, int rows, int cols);
void translate_toroidal_array(Point *p, Direction dir, int rows, int cols);
std::vector<Point> get_neighbours_inside_grid(Point *point, int rows, int cols);
std::vector<Point> get_adjacent_neighbours_inside_grid(Point *point, int rows,
                                                       int cols);
/**
 * Useful for implementing grids with toroidal array geometry. For a given point
 * touching the edge of the grid, it returns its 'neighbours' on the other
 * side of the grid. For instance, for a point touching the top edge of the
 * grid, three points along the bottom edge of the grid will be returned
 * together with the five cells that are neighbouring the current cell. */
std::vector<Point> get_neighbours_toroidal_array(Point *point, int rows,
                                                 int cols);

bool is_adjacent(Point *p1, Point *p2);
