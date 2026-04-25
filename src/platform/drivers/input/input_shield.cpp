#include <Arduino.h>
#include "input_shield.hpp"

/** Pins controlling the joystick */
#define STICK_Y_PIN 16
#define STICK_X_PIN 17

bool ArduinoInputShield::poll_for_input(Direction *input)
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

bool ArduinoInputShield::poll_for_input(Action *input)
{
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

void ArduinoInputShield::setup()
{
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);
}
