#include "adafruit_mini_controller.hpp"
#include "joystick_controller.hpp"
#include "../../logging.hpp"

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

        // Serial.println(buttons, BIN);

        if (!(buttons & (1UL << BUTTON_A))) {
                *input = Action::RED;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_B))) {
                *input = Action::GREEN;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_Y))) {
                *input = Action::BLUE;
                return true;
        }
        if (!(buttons & (1UL << BUTTON_X))) {
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
