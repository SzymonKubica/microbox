#ifdef EMULATOR
#include "../interface/delay.hpp"
#include <chrono>
#include <thread>

/**
 * Delay provider implementation for the emulator platform.
 */
class EmulatorDelay : public DelayProvider
{
        void delay_ms(int ms) override
        {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
};
#endif
