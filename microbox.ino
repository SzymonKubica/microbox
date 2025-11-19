#include "src/common/platform/arduino/joystick_controller.hpp"
#include "src/common/platform/arduino/wifi_provider.cpp"
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

void setup(void)
{
        // Initialise serial port for debugging
        Serial.begin(115200);
        while (!Serial) {
                // wait for serial port to connect
        }

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

        Platform platform = {.display = &display,
                             .directional_controllers = &controllers,
                             .action_controllers = &action_controllers,
                             .delay_provider = delay_provider,
                             .persistent_storage = &persistent_storage,
                             .wifi_provider = wifi_provider

        };

        select_game(&platform);
}
