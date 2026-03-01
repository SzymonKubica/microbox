#include "Adafruit_seesaw.h"
#include <Wire.h>

#if !defined(ARDUINO_UNO_Q)
#include <EEPROM.h>
#endif

#include "src/common/platform/arduino/joystick_controller.hpp"
#include "src/common/platform/arduino/wifi_provider.cpp"
#include "src/common/platform/arduino/arduino_http_client.hpp"
#include "src/common/platform/interface/wifi.hpp"
#include "src/common/platform/arduino/keypad_controller.hpp"
#include "src/common/platform/arduino/adafruit_mini_controller.hpp"
#include "src/common/platform/arduino/lcd_display.hpp"
#include "src/common/platform/arduino/arduino_secrets.hpp"
#include "src/common/platform/arduino/arduino_time_provider.cpp"
#include "src/common/platform/interface/persistent_storage.hpp"

#include "src/games/game_menu.hpp"
#include "src/games/2048.hpp"

LcdDisplay display;
JoystickController *joystick_controller;
KeypadController *keypad_controller;
AdafruitController *adafruit_controller;
PersistentStorage persistent_storage;

Adafruit_seesaw ss(&Wire1);

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
        pinMode(IRQ_PIN, INPUT);
#endif
        return true;
}

bool adafruit_gamepad_available;
void setup(void)
{

        // Initialise serial port for debugging
        Serial.begin(115200);

        adafruit_gamepad_available = setup_adafruit_seesaw_i2c_connection();
        // eeprom_erase();

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

        std::vector<DirectionalController *> controllers = {
            joystick_controller};
        std::vector<ActionController *> action_controllers = {
            keypad_controller};

        if (adafruit_gamepad_available) {
                adafruit_controller = new AdafruitController(&ss);
                action_controllers.push_back(adafruit_controller);
                controllers.push_back(adafruit_controller);
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
