#ifdef EMULATOR
#include "../interface/time_provider.hpp"
#include <chrono>
#include <thread>

/**
 * Delay provider implementation for the emulator platform.
 */
class EmulatorTimeProvider : public TimeProvider
{
        void delay_ms(int ms) override
        {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }

        long milliseconds() override
        {
                return std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                    .count();
        }
};
#endif
