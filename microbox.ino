#include <Wire.h>

#include <EEPROM.h>
#include <nvs_flash.h>

#include "src/common/platform/arduino/joystick_controller.hpp"
#include "src/common/platform/arduino/wifi_provider.cpp"
#include "src/common/platform/arduino/arduino_http_client.hpp"
#include "src/common/platform/interface/wifi.hpp"
#include "src/common/platform/arduino/keypad_controller.hpp"
#include "src/common/platform/arduino/adafruit_mini_controller.hpp"
#if defined(WAVESHARE_1_69_INCH_LCD)
#include "src/common/platform/arduino/lcd_display_1_69_inch.hpp"
#endif
#if defined(WAVESHARE_2_4_INCH_LCD)
#include "src/common/platform/arduino/lcd_display_2_4_inch.hpp"
#endif
#include "src/common/platform/arduino/arduino_secrets.hpp"
#include "src/common/platform/arduino/arduino_time_provider.cpp"
#include "src/common/platform/interface/persistent_storage.hpp"

#include "src/games/game_menu.hpp"
#include "src/games/2048.hpp"

#include "Adafruit_seesaw.h"

LcdDisplay display;
JoystickController *joystick_controller;
KeypadController *keypad_controller;
AdafruitController *adafruit_controller;
PersistentStorage persistent_storage;

#if 1
Adafruit_seesaw ss(&Wire);
// #define IRQ_PIN 5
#endif

/**
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

bool setup_adafruit_seesaw_i2c_connection()
{

#if 1
        Serial.println("Setting up seesaw I2C interface...");

        if (!ss.begin(0x50)) {
                Serial.println("ERROR! seesaw not found");
                return false;
        }
        Serial.println("seesaw started");
        uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
        if (version != 5743) {
                Serial.print("Wrong firmware loaded? ");
                Serial.println(version);
                return false;
        }

        Serial.println("Found Product 5743");

        ss.pinModeBulk(button_mask, INPUT_PULLUP);
        ss.setGPIOInterrupts(button_mask, 1);

#if defined(IRQ_PIN)
        // pinMode(IRQ_PIN, INPUT);
#endif
#endif
        return true;
}

// ESP32 OVERRIDE
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

bool adafruit_gamepad_available;
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

#ifdef ARDUINO_ARCH_ESP32
        // On esp32 EEPROM is simulated in the flash storage. Because of this
        // we need to initialize it here explicitly
        EEPROM.begin(512);
#endif

        adafruit_gamepad_available = setup_adafruit_seesaw_i2c_connection();

        // Set up bins neeeded for controllers
        pinMode(STICK_BUTTON_PIN, INPUT);
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);

        // Initializes the source of randomness from the
        // noise present on the first digital pin
        initialize_randomness_seed(analogRead(0));

        persistent_storage = PersistentStorage{};

        // Initialize the hardware LCD display
        display = LcdDisplay{};
        display.setup();
}

int last_x = 0, last_y = 0;
void loop(void)
{

        Serial.println("Game console started.");

        // Initialize game platform components
        joystick_controller =
            new JoystickController((int (*)(unsigned char))&analogRead);
        keypad_controller =
            new KeypadController((int (*)(unsigned char))&digitalRead);

        std::vector<DirectionalController *> controllers = {};
        std::vector<ActionController *> action_controllers = {};

        if (adafruit_gamepad_available) {
#if 1
                // Only R4 wifi has the stemma qt port for the Wire1 SPI
                // interface. On R4 Minima Adafruit seesaw is not defined.
                adafruit_controller = new AdafruitController(&ss);
                action_controllers.push_back(adafruit_controller);
                controllers.push_back(adafruit_controller);
#endif
        }

        TimeProvider *time_provider =
            new ArduinoTimeProvider((void (*)(int))&delay);
        WifiProvider *wifi_provider = new ArduinoWifiProvider{};
        ArduinoHttpClient *client = new ArduinoHttpClient();

        Platform platform = {.display = &display,
                             .directional_controllers = &controllers,
                             .action_controllers = &action_controllers,
                             .time_provider = time_provider,
                             .persistent_storage = &persistent_storage,
                             .wifi_provider = wifi_provider,
                             .client = client};

        select_game(&platform);
}
