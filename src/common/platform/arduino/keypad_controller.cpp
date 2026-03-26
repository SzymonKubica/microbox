#include "keypad_controller.hpp"
#include <Arduino.h>

bool KeypadController::poll_for_input(Action *input) {
        int leftButton = digitalRead(LEFT_BUTTON_PIN);
        int downButton = digitalRead(DOWN_BUTTON_PIN);
        int upButton = digitalRead(UP_BUTTON_PIN);
        int rightButton = digitalRead(RIGHT_BUTTON_PIN);

        if (!leftButton) {
                *input = Action::BLUE;
                return true;
        }
        if (!downButton) {
                *input = Action::GREEN;
                return true;
        }
        if (!upButton) {
                *input = Action::YELLOW;
                return true;
        }
        if (!rightButton) {
                *input = Action::RED;
                return true;
        }
        return false;
}

void KeypadController::setup() {
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);
}
