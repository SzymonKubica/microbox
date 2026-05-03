#pragma once
#ifndef EMULATOR
#include "Adafruit_seesaw.h"
#include "../../interface/controller.hpp"

#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16
extern uint32_t button_mask;

class MiniGamepadController : public DirectionalController,
                              public ActionController
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
        std::optional<Direction> poll_for_direction() override;

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
        std::optional<Action> poll_for_action() override;

        /**
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         */
        void setup() override;

        MiniGamepadController(Adafruit_seesaw *ss) : ss(ss) {}

      private:
        Adafruit_seesaw *ss;
};
#endif
