#include <EEPROM.h>
#include "src/common/platform/arduino/joystick_controller.hpp"
#include "src/common/platform/arduino/wifi_provider.cpp"
#include "src/common/platform/arduino/arduino_http_client.hpp"
#include "src/common/platform/interface/wifi.hpp"
#include "src/common/platform/arduino/keypad_controller.hpp"
#include "src/common/platform/arduino/lcd_display.hpp"
#include "src/common/platform/arduino/arduino_secrets.hpp"
#include "src/common/platform/arduino/arduino_delay.cpp"
#include "src/common/platform/interface/persistent_storage.hpp"

#include "src/games/game_menu.hpp"
#include "src/games/2048.hpp"

LcdDisplay display;
JoystickController *joystick_controller;
KeypadController *keypad_controller;
PersistentStorage persistent_storage;

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
        // Iterate through all EEPROM addresses and write 0 to each
        for (int i = 0; i < EEPROM.length(); i++) {
                EEPROM.write(i, 0);
        }
}

void setup(void)
{
        // Initialise serial port for debugging
        Serial.begin(115200);

        //eeprom_erase();

        // Set up bins neeeded for controllers
        pinMode(STICK_BUTTON_PIN, INPUT);
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);

        // Initializes the source of randomness from the
        // noise present on the first digital pin
        initialize_randomness_seed(analogRead(0));

        // Initialize game platrorm components
        joystick_controller =
            new JoystickController((int (*)(unsigned char))&analogRead);
        keypad_controller =
            new KeypadController((int (*)(unsigned char))&digitalRead);

        persistent_storage = PersistentStorage{};

        // Initialize the hardware LCD display
        display = LcdDisplay{};
        display.setup();
}

void loop(void)
{
        Serial.println("Game console started.");

        std::vector<DirectionalController *> controllers(1);
        controllers[0] = joystick_controller;
        std::vector<ActionController *> action_controllers(1);
        action_controllers[0] = keypad_controller;

        DelayProvider *delay_provider = new ArduinoDelay((void (*)(int))&delay);
        WifiProvider *wifi_provider = new ArduinoWifiProvider{};
        ArduinoHttpClient *client = new ArduinoHttpClient();

        Platform platform = {.display = &display,
                             .directional_controllers = &controllers,
                             .action_controllers = &action_controllers,
                             .delay_provider = delay_provider,
                             .persistent_storage = &persistent_storage,
                             .wifi_provider = wifi_provider,
                             .client = client};

        select_game(&platform);
}
