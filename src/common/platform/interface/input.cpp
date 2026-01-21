#include "input.hpp"

const char *direction_to_str(Direction direction)
{
        switch (direction) {
        case UP:
                return "Up";
        case LEFT:
                return "Left";
        case RIGHT:
                return "Right";
        case DOWN:
                return "Down";
        default:
                return "Unknown";
        };
};

bool is_opposite(const Direction direction, const Direction other_direction)
{
        switch (direction) {
        case UP:
                return other_direction == DOWN;
        case RIGHT:
                return other_direction == LEFT;
        case DOWN:
                return other_direction == UP;
        case LEFT:
                return other_direction == RIGHT;
        }
        return false;
}

Direction get_opposite(const Direction direction)
{
        switch (direction) {
        case UP:
                return DOWN;
        case RIGHT:
                return LEFT;
        case DOWN:
                return UP;
        case LEFT:
                return RIGHT;
        }
        // garbage in, garbage out. If we get an unrecognized direction,
        // we return it back.
        return direction;
}

const char *action_to_str(Action action)
{
        switch (action) {
        case YELLOW:
                return "Yellow";
        case RED:
                return "Red";
        case GREEN:
                return "Green";
        case BLUE:
                return "Blue";
        default:
                return "Unknown";
        };
};

const Direction action_to_direction(Action action)
{
        switch (action) {
        case YELLOW:
                return UP;
        case RED:
                return RIGHT;
        case GREEN:
                return DOWN;
        case BLUE:
                return LEFT;
        default:
                // unreachable
                return UP;
        };
}
