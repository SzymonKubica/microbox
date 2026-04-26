#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
#pragma once
#include "time_provider.hpp"
#include "Arduino.h"
void ArduinoTimeProvider::delay_ms(int ms) { delay(ms); }
long ArduinoTimeProvider::milliseconds() { return millis(); }
#endif
