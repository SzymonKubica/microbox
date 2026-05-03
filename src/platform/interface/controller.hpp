#pragma once

#include "input.hpp"
#include <stdlib.h>
#include <vector>
#include <optional>

class DirectionalController
{
      public:
        /**
         * For a given controller, this function will inspect its state to
         * determine if an input is being entered. Note that for physical
         * controllers, this function only tests for the state of the controller
         * right now (it doesn't poll for a period of time). Becuase of this,
         * this function should be called in a loop if we want the system to
         * wait for the user to provide input.
         *
         * If no output is registered, an empty optional will be returned.
         */
        virtual std::optional<Direction> poll_for_direction() = 0;

        /**
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         */
        virtual void setup() = 0;
};

class ActionController
{
      public:
        /**
         * For a given controller, this function will inspect its state to
         * determine if an input is being entered. Note that for physical
         * controllers, this function only tests for the state of the controller
         * right now (it doesn't poll for a period of time). Becuase of this,
         * this function should be called in a loop if we want the system to
         * wait for the user to provide input.
         *
         * If no output is registered, an empty optional will be returned.
         */
        virtual std::optional<Action> poll_for_action() = 0;

        /**
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         */
        virtual void setup() = 0;
};

/**
 * Checks if any of the controllers has recorded user input.
 * If multiple controllers have registered an input during the execution of this
 * function, the `Direction` reported by the first one will take precedence
 */
std::optional<Direction>
poll_directional_input(const std::vector<DirectionalController *> &controllers);

/**
 * Checks if any of the controllers has recorded user action input.
 * If multiple controllers have registered an input during the execution of this
 * function, the `Action` reported by the first one will take precedence
 */
std::optional<Action>
poll_action_input(const std::vector<ActionController *> &controllers);
