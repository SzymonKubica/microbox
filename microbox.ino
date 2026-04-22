#include <Wire.h>

#include <EEPROM.h>
#include <nvs_flash.h>

// TODO: all arduino R4 specific items were commented out.
//#include "src/platform/drivers/input/input_shield.hpp"
// TODO: move those esp32 specific includes to a separate esp32 platform target
// file
#include "src/platform/target/microbox_v2.hpp"
//#if defined(WAVESHARE_1_69_INCH_LCD)
//#include "src/platform/drivers/display/lcd_display_1_69_inch.hpp"
//#endif

#include "src/games/game_menu.hpp"
#include "src/games/2048.hpp"
#include "src/games/brightness.hpp"

//ArduinoInputShield *input_shield;

/**
 * TODO: is this even requied anymore given that now persistent storage
 *       structs are properly versioned.
 * When making breaking changes to the persistent storage used for game
 * configs, sometimes it is required to clean EEPROM before the new schema
 * of the config values can be written to it. This function allows for that.
 *
 * When performing the erase cycle, add this function to the setup method and
 * flush this temporary version of the program to your arduino. Once it runs,
 * your EEPROM will be erased. Note that you should rollback this change and
 * flush a new version of the code without this erase function. EEPROM memory
 * has a finite number of times that we can write to it, so you need to ensure
 * that it doesn't get erased each time you run the setup() function.
 */
void eeprom_erase()
{
#if !defined(ARDUINO_UNO_Q)
        // Iterate through all EEPROM addresses and write 0 to each
        for (int i = 0; i < EEPROM.length(); i++) {
                EEPROM.write(i, 0);
        }
#endif
}

// ESP32 OVERRIDE
SET_LOOP_TASK_STACK_SIZE(32 * 1024);

bool adafruit_gamepad_available;

void rgb_blink_task(void *parameter)
{
        const int rgb_mode = 0;
        const int off = 1;
        const int flashlight = 2;
        int current = 1;
        while (true) {
                if (digitalRead(38) == LOW) {
                        current = (current + 1) % 3;
                }
#ifdef RGB_BUILTIN
                switch (current) {
                case rgb_mode:
                        digitalWrite(RGB_BUILTIN,
                                     HIGH); // Turn the RGB LED white
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        digitalWrite(RGB_BUILTIN, LOW); // Turn the RGB LED off
                        vTaskDelay(1000 / portTICK_PERIOD_MS);

                        rgbLedWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        rgbLedWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        rgbLedWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        rgbLedWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        break;
                case off:
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        break;
                case flashlight:
                        digitalWrite(RGB_BUILTIN,
                                     HIGH); // Turn the RGB LED white
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
#endif
        }
}

Platform *platform;

void setup(void)
{

        // Initialise serial port for debugging
        Serial.begin(115200);

        // eeprom_erase();
        bool erase_flash = false;
        if (erase_flash) {
                nvs_flash_erase();
                nvs_flash_init();
        }

        platform = initialize_platform();

        setup(platform);

        // Set up bins neeeded for controllers

        // Initializes the source of randomness from the
        // noise present on the first digital pin
        initialize_randomness_seed(analogRead(0));

        xTaskCreate(rgb_blink_task,            // function
                    "RGB diode blinking task", // name
                    2048,                      // stack size
                    NULL,                      // parameter
                    1,                         // priority
                    NULL                       // task handle
        );
}

int last_x = 0, last_y = 0;
void loop(void)
{

        Serial.println("Game console started.");

        //        input_shield = new ArduinoInputShield();
        //        input_shield->setup();

        set_brightness_from_storage(platform->persistent_storage);

        while (true) {
                select_game(platform);
        }
}
