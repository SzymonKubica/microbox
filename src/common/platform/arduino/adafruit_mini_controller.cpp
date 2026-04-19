#include "adafruit_mini_controller.hpp"
#include "../../logging.hpp"

/**
 * The joystick reports the current position using two potentiometers. Those
 * are read using analog pins that return values in range 0-1023. The two
 * constants below control how quickly the joystick registers input.
 */
#define HIGH_THRESHOLD 900
#define LOW_THRESHOLD 100

#define TAG "AdafruitController"

uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) |
                       (1UL << BUTTON_START) | (1UL << BUTTON_A) |
                       (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

bool AdafruitController::poll_for_input(Direction *input)
{
        // Reverse x/y values to match joystick orientation
        int x = ss->analogRead(14);
        int y = ss->analogRead(15);

        if (x < LOW_THRESHOLD) {
                *input = Direction::RIGHT;
                return true;
        }
        if (x > HIGH_THRESHOLD) {
                *input = Direction::LEFT;
                return true;
        }
        if (y < LOW_THRESHOLD) {
                *input = Direction::UP;
                return true;
        }
        if (y > HIGH_THRESHOLD) {
                *input = Direction::DOWN;
                return true;
        }
        return false;
};

bool AdafruitController::poll_for_input(Action *input)
{

        uint32_t buttons = ss->digitalReadBulk(button_mask);
        // reject impossible results due to I2C delays caused by heavy SPI load.
        if (buttons == 0 || buttons == 0xFFFFFFFF)
                return false;

        if (!(buttons & (1UL << BUTTON_A))) {
                LOG_DEBUG(TAG, "A button pressed.");
                *input = Action::RED;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_B))) {
                LOG_DEBUG(TAG, "B button pressed.");
                *input = Action::GREEN;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_Y))) {
                LOG_DEBUG(TAG, "Y button pressed.");
                *input = Action::BLUE;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_X))) {
                LOG_DEBUG(TAG, "X button pressed.");
                *input = Action::YELLOW;
                return true;
        }
        // Ignore the extra buttons for now
        if (!(buttons & (1UL << BUTTON_SELECT))) {
                LOG_DEBUG(TAG, "Button SELECT pressed, but we ignore it.");
        }
        if (!(buttons & (1UL << BUTTON_START))) {
                LOG_DEBUG(TAG, "Button START pressed, but we ignore it.");
        }
        return false;
}

void AdafruitController::setup() {}
