// platform.h

#pragma once

/**
 * This is supposed to be the prototype of the platform resolution header.
 * this should be the single point where all platform-specific pieces of code
 */

#if defined(PLATFORM_STM32)
    #include "platform_stm32.h"

#elif defined(PLATFORM_AVR)
    #include "platform_avr.h"

#elif defined(PLATFORM_ESP32)
    #include "platform_esp32.h"
#else
#endif

/*
Running list of things that are platform-specific:
- wifi:
    - different on Arduino
    - different on Emulator
    - different on esp32

 */
