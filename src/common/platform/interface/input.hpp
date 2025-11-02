#pragma once
/**
 * Enum modeling the four possible directions of user input.
 */
typedef enum Direction { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 } Direction;

bool is_opposite(const Direction direction, const Direction other_direction);
/**
 * Enum for the four possible 'user actions' that are mapped to the colors of
 * the directional keypad on the Arduino physical controller shield.
 */
typedef enum Action { YELLOW = 0, RED = 1, GREEN = 2, BLUE = 3 } Action;

const char *direction_to_str(Direction direction);
const char *action_to_str(Action action);
const Direction action_to_direction(Action action);
