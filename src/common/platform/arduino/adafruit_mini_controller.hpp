#pragma once
#ifndef EMULATOR
#include <cstdint>
typedef uint8_t BitOrder;
#define MSBFIRST SPI_MSBFIRST
#define LSBFIRST SPI_LSBFIRST
#include "Adafruit_seesaw.h"
#include "../interface/controller.hpp"

#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16
extern uint32_t button_mask;

class AdafruitController : public DirectionalController, public ActionController
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
        bool poll_for_input(Direction *input) override;

        /**
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         *
         * DEPRECATED: This will likely not be necessary in the future. Original
         * plan was to initialize the pins there. The problem is that the
         * function for doing this is overloaded so we cannot pass it as a
         * pointer without contextual information (makes sense, it won't be
         * possible to infer which function to call).
         */
        void setup() override;

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
        bool poll_for_input(Action *input) override;

        AdafruitController(Adafruit_seesaw *ss) : ss(ss) {}

      private:
        Adafruit_seesaw *ss;
};
#endif
