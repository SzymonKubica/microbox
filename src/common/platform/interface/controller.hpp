#pragma once

#include "input.hpp"
#include <stdlib.h>
#include <vector>

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
         * If an input is registered, it will be written into the `Direction
         * *input` parameter and `true` will be returned.
         *
         * If no input is registered, this function returns false and the
         * direction pointer remains unchanged.
         */
        virtual bool poll_for_input(Direction *input) = 0;

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
         * If an input is registered, it will be written into the `Action
         * *input` parameter and `true` will be returned.
         *
         * If no input is registered, this function returns false and the
         * direction pointer remains unchanged.
         */
        virtual bool poll_for_input(Action *input) = 0;

        /**
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         */
        virtual void setup() = 0;
};

extern bool
poll_directional_input(std::vector<DirectionalController *> *controllers,
                             Direction *registered_dir);

extern bool
poll_action_input(std::vector<ActionController *> *controllers,
                             Action *registered_action);
