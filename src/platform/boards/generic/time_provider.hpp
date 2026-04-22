#if !defined(EMULATOR)
#pragma once
#include "../../interface/time_provider.hpp"
class ArduinoTimeProvider : public TimeProvider
{
        void delay_ms(int ms) override;
        long milliseconds() override;
};
#endif
