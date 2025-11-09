#include "../interface/delay.hpp"
class ArduinoDelay : public DelayProvider
{
        void delay_ms(int ms) override { delay(ms); }

      public:
        /**
         * The function pointer to the Arduino delay needs to be passed in the
         * .ino file. The reason is that those functions cannot be imported
         * in C++ sources directly.
         */
        ArduinoDelay(void (*delay_)(int)) : delay(delay_) {}

      private:
        /**
         * The function pointer to the actual arduino delay function.
         */
        void (*delay)(int);
};
