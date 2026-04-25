#pragma once
#include "../interface/platform.hpp"

Platform *initialize_platform();
void setup(Platform *platform);

/**
 * We need to increase the loop stack size on esp32 to run more
 * intensive workloads. Because of this we need to implement this function
 * to return the overridden stack size.
 *
 * The naming convention is `camelCase` as we need to override the definition
 * from the arduino core exactly, see here for reference;
 * https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Arduino.h
 */
size_t getArduinoLoopTaskStackSize();
