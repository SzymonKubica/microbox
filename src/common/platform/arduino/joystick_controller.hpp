#include "../interface/controller.hpp"

/**
 * The joystick reports the current position using two potentiometers. Those
 * are read using analog pins that return values in range 0-1023. The two
 * constants below control how quickly the joystick registers input.
 */
#define HIGH_THRESHOLD 900
#define LOW_THRESHOLD 100

/** Pins controlling the joystick */
#define A0 13
#define STICK_BUTTON_PIN A0
#define STICK_Y_PIN 16
#define STICK_X_PIN 17

class JoystickController : public DirectionalController
{
      public:
        /**
         * For a given controllers, this function will inspect its state to
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
         * DEPRECATED: This will likely not be necessary in the future. Original plan
         * was to initialize the pins there. The problem is that the function for
         * doing this is overloaded so we cannot pass it as a pointer without
         * contextual information (makes sense, it won't be possible to infer
         * which function to call).
         */
        void setup() override;

        JoystickController(int (*analog_read_)(unsigned char))
            : analog_read(analog_read_)
        {
        }

      private:
        /**
         * The analog read function that is provided by the Arduino core layer.
         * Allows for reading the voltage on the potentiometer that is
         * controlled by the joystick.
         *
         * This is to be passed in when constructing the joystick controller.
         * The reason is that we cannot import the Arduino specific functions
         * inside of the C++ sources.
         *
         */
        int (*analog_read)(unsigned char);
};
