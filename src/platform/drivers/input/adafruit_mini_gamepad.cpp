#include "adafruit_mini_gamepad.hpp"
#include "../../../common/logging.hpp"
#include <optional>

/**
 * The joystick reports the current position using two potentiometers. Those
 * are read using analog pins that return values in range 0-1023. The two
 * constants below control how quickly the joystick registers input.
 */
#define HIGH_THRESHOLD 900
#define LOW_THRESHOLD 100

#define TAG "MiniGamepadController"

uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) |
                       (1UL << BUTTON_START) | (1UL << BUTTON_A) |
                       (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

std::optional<Direction> MiniGamepadController::poll_for_direction()
{
        // Reverse x/y values to match joystick orientation
        int x = ss->analogRead(14);
        int y = ss->analogRead(15);

        if (x < LOW_THRESHOLD)
                return Direction::RIGHT;
        if (x > HIGH_THRESHOLD)
                return Direction::LEFT;
        if (y < LOW_THRESHOLD)
                return Direction::UP;
        if (y > HIGH_THRESHOLD)
                return Direction::DOWN;
        return std::nullopt;
};

std::optional<Action> MiniGamepadController::poll_for_action()
{

        uint32_t buttons = ss->digitalReadBulk(button_mask);
        // reject impossible results due to I2C delays caused by heavy SPI load.
        if (buttons == 0 || buttons == 0xFFFFFFFF)
                return std::nullopt;

        if (!(buttons & (1UL << BUTTON_A))) {
                LOG_DEBUG(TAG, "A button pressed.");
                return Action::RED;
        }
        if (!(buttons & (1UL << BUTTON_B))) {
                LOG_DEBUG(TAG, "B button pressed.");
                return Action::GREEN;
        }
        if (!(buttons & (1UL << BUTTON_Y))) {
                LOG_DEBUG(TAG, "Y button pressed.");
                return Action::BLUE;
        }
        if (!(buttons & (1UL << BUTTON_X))) {
                LOG_DEBUG(TAG, "X button pressed.");
                return Action::YELLOW;
        }
        // Ignore the extra buttons for now
        if (!(buttons & (1UL << BUTTON_SELECT))) {
                LOG_DEBUG(TAG, "Button SELECT pressed, but we ignore it.");
        }
        if (!(buttons & (1UL << BUTTON_START))) {
                LOG_DEBUG(TAG, "Button START pressed, but we ignore it.");
        }
        return std::nullopt;
}

void MiniGamepadController::setup() {}
