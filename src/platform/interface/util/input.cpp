#include "../input.hpp"

bool is_opposite(const Direction direction, const Direction other_direction)
{
        switch (direction) {
        case Direction::UP:
                return other_direction == Direction::DOWN;
        case Direction::RIGHT:
                return other_direction == Direction::LEFT;
        case Direction::DOWN:
                return other_direction == Direction::UP;
        case Direction::LEFT:
                return other_direction == Direction::RIGHT;
        }
        return false;
}

Direction get_opposite(const Direction direction)
{
        switch (direction) {
        case Direction::UP:
                return Direction::DOWN;
        case Direction::RIGHT:
                return Direction::LEFT;
        case Direction::DOWN:
                return Direction::UP;
        case Direction::LEFT:
                return Direction::RIGHT;
        }
        // garbage in, garbage out. If we get an unrecognized direction,
        // we return it back.
        return direction;
}

const Direction action_to_direction(Action action)
{
        switch (action) {
        case Action::YELLOW:
                return Direction::UP;
        case Action::RED:
                return Direction::RIGHT;
        case Action::GREEN:
                return Direction::DOWN;
        case Action::BLUE:
                return Direction::LEFT;
        default:
                // unreachable
                return Direction::UP;
        };
}
