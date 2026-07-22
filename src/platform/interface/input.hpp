#pragma once
/**
 * The four possible directions of user input.
 */
#include <cstdint>
#include <utility>
#include "../../common/string_enum.hpp"
enum class Direction : uint8_t { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };

bool is_opposite(const Direction direction, const Direction other_direction);
Direction get_opposite(const Direction direction);

/**
 * The four possible 'user actions' that are mapped to the colors of
 * the directional keypad on the Arduino physical controller shield.
 */
enum class Action : uint8_t { YELLOW = 0, RED = 1, GREEN = 2, BLUE = 3 };

const Action CONFIRM_ACTION = Action::GREEN;
const Action BACK_ACTION = Action::RED;
const Action HELP_ACTION = Action::YELLOW;
const Action FORWARD_ACTION = Action::BLUE;

const Direction action_to_direction(Action action);

namespace ActionStr
{
constexpr std::pair<Action, const char *> TABLE[] = {
    {Action::YELLOW, "Yellow"},
    {Action::RED, "Red"},
    {Action::GREEN, "Green"},
    {Action::BLUE, "Blue"},
};

constexpr const char *to_cstr(Action action)
{
        return StrEnum::to_cstr(action, TABLE);
}
} // namespace ActionStr

namespace DirectionStr
{
constexpr std::pair<Direction, const char *> TABLE[] = {
    {Direction::UP, "Up"},
    {Direction::DOWN, "Down"},
    {Direction::LEFT, "Left"},
    {Direction::RIGHT, "Right"},
};
constexpr const char *to_cstr(Direction action)
{
        return StrEnum::to_cstr(action, TABLE);
}
} // namespace DirectionStr
