#include "src/menu.hpp"
#include "src/games/brightness.hpp"
#include "src/platform/target/target_resolution.hpp"
#include "src/common/logging.hpp"

#define TAG "microbox_entrypoint"


/**
 * The platform object that bundles up all handles to the platform-specific
 * things like displays, controllers, persistent storage or WiFi connectivity
 * infra. The idea is that depending on the target version of MicroBox, we
 * initialize a different platform and the rest of the code is oblivious to the
 * internals of the platform that it is running on.
 */
Platform *platform;

void common_setup(void);
void setup(void)
{
        LOG_INFO(TAG, "Initializing platform...");
        common_setup();
        platform = initialize_platform();
        setup(platform);
}

#define SERIAL_BAUD_RATE 115200
/**
 * Common setup that applies to all platforms (regardless of arduino / esp32).
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
        LOG_INFO(TAG, "MicroBox started!");
        set_brightness_from_storage(platform->persistent_storage);

        while (true) {
                select_app_and_run(platform);
        }
}
