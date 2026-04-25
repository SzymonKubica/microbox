#include <Wire.h>

#include "src/platform/target/microbox_v2.hpp"

#include "src/games/game_menu.hpp"
#include "src/games/brightness.hpp"

#define SERIAL_BAUD_RATE 115200

/**
 * The platform object that bundles up all handles to the platform-specific
 * things like displays, controllers, persistent storage or WiFi connectivity
 * infra. The idea is that depending on the target version of MicroBox, we
 * initialize a different platform and the rest of the code is oblivious to the
 * internals of the platform that it is running on.
 */
Platform *platform;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

void common_setup(void);
void setup(void)
{
        Serial.println("Initializing platform...");
        common_setup();
        platform = initialize_platform();
        setup(platform);
}

/**
 * Common setup functions that applies to all platforms (arduino / esp32)
 */
void common_setup(void)
{
        Serial.begin(SERIAL_BAUD_RATE);
        // Initializes the source of randomness from the
        // electrical noise present on the first D0 digital pin
        srand(analogRead(0));
}

void loop(void)
{
        Serial.println("MicroBox started!");
        set_brightness_from_storage(platform->persistent_storage);

        while (true) {
                select_game(platform);
        }
}
