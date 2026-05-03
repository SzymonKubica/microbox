#ifdef EMULATOR
#include "../interface/controller.hpp"

class SfmlHjklInputController : public DirectionalController
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
         * Setup function used for e.g. initializing pins of the controller.
         * This is to be called only once inside of the `setup` Arduino
         * function.
         */
        void setup() override;
};
#endif
