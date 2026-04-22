#if !defined(EMULATOR)
#include "time_provider.hpp"
#include "Arduino.h"
void ArduinoTimeProvider::delay_ms(int ms) { delay(ms); }
long ArduinoTimeProvider::milliseconds() { return millis(); }
#endif
