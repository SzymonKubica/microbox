#include <Arduino.h>
#include "joystick_controller.hpp"

/** Pins controlling the joystick */
#define STICK_Y_PIN 16
#define STICK_X_PIN 17

bool JoystickController::poll_for_input(Direction *input)
{

        int x_val = analogRead(STICK_X_PIN);
        int y_val = analogRead(STICK_Y_PIN);

        if (x_val < LOW_THRESHOLD) {
                *input = Direction::RIGHT;
                return true;
        }
        if (x_val > HIGH_THRESHOLD) {
                *input = Direction::LEFT;
                return true;
        }
        if (y_val < LOW_THRESHOLD) {
                *input = Direction::UP;
                return true;
        }
        if (y_val > HIGH_THRESHOLD) {
                *input = Direction::DOWN;
                return true;
        }
        return false;
}

void JoystickController::setup() {  }
