#include "power_manager.hpp"
#if defined(ARDUINO_ARCH_ESP32)
#include "esp_sleep.h"
#include "Arduino.h"

void EspPowerManager::enter_deep_sleep() const
{
#define DEV_BL_PIN 4
        // We are entering deep sleep and shutting down the main CPU.
        // The intent is that resetting the console is the only way to
        // wake up now.
        gpio_hold_en((gpio_num_t)DEV_BL_PIN);
        gpio_deep_sleep_hold_en();
        esp_deep_sleep_start();
}
#endif
