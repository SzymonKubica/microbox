#ifndef EMULATOR
#include "../interface/time_provider.hpp"
#include "Arduino.h"
class ArduinoTimeProvider : public TimeProvider
{
        void delay_ms(int ms) override { delay(ms); }
        long milliseconds() override { return millis(); }

      public:
        /**
         * The function pointer to the Arduino delay needs to be passed in the
         * .ino file. The reason is that those functions cannot be imported
         * in C++ sources directly.
         */
        ArduinoTimeProvider(void (*delay_)(int)) : delay(delay_) {}

      private:
        /**
         * The function pointer to the actual arduino delay function.
         */
        void (*delay)(int);
};
#endif
