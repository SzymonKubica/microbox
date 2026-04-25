#if defined(ARDUINO_ARCH_ESP32) | defined(ARDUINO_UNOR4_WIFI)
#pragma once
#include "../../interface/time_provider.hpp"
class ArduinoTimeProvider : public TimeProvider
{
        void delay_ms(int ms) override;
        long milliseconds() override;
};
#endif
