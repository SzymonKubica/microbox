#include <Arduino.h>
#include "input_shield.hpp"
#include <optional>

/** Pins controlling the joystick */
#define STICK_Y_PIN 16
#define STICK_X_PIN 17

std::optional<Direction> ArduinoInputShield::poll_for_direction()
{

        int x_val = analogRead(STICK_X_PIN);
        int y_val = analogRead(STICK_Y_PIN);

        if (x_val < LOW_THRESHOLD)
                return Direction::RIGHT;
        if (x_val > HIGH_THRESHOLD)
                return Direction::LEFT;
        if (y_val < LOW_THRESHOLD)
                return Direction::UP;
        if (y_val > HIGH_THRESHOLD)
                return Direction::DOWN;
        return std::nullopt;
}

std::optional<Action> ArduinoInputShield::poll_for_action()
{
        int leftButton = digitalRead(LEFT_BUTTON_PIN);
        int downButton = digitalRead(DOWN_BUTTON_PIN);
        int upButton = digitalRead(UP_BUTTON_PIN);
        int rightButton = digitalRead(RIGHT_BUTTON_PIN);

        if (!leftButton)
                return Action::BLUE;
        if (!downButton)
                return Action::GREEN;
        if (!upButton)
                return Action::YELLOW;
        if (!rightButton)
                return Action::RED;
        return std::nullopt;
}

void ArduinoInputShield::setup()
{
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);
}
