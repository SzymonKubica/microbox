#include <Arduino_FreeRTOS.h>
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

/**
 * We wrap the RTOS delay function so that it matches the signature expected
 * by the delay provider.
 */
void rtos_delay(int ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }

void wifi_connection_task(void *pv_parameters)
{

        Serial.println("Trying to connect to Wi-Fi.");
        ArduinoWifiProvider wifi_provider = ArduinoWifiProvider{};
        auto wifi_data =
            wifi_provider.connect_to_network(SECRET_SSID, SECRET_PASS);

        if (wifi_data.has_value()) {
                WifiData *data = wifi_provider.get_wifi_data();
                char *data_string = get_wifi_data_string(data);
                Serial.println(data_string);
        }
        vTaskDelete(NULL);
}

void main_task(void *pv_parameters)
{
        // We need to ensure that those are on the actual stack of the task.
        LcdDisplay display;
        JoystickController *joystick_controller;
        KeypadController *keypad_controller;
        PersistentStorage persistent_storage;

        joystick_controller =
            new JoystickController((int (*)(unsigned char))&analogRead);

        keypad_controller =
            new KeypadController((int (*)(unsigned char))&digitalRead);

        persistent_storage = PersistentStorage{};

        // Initialize the hardware LCD display
        display = LcdDisplay{};
        display.setup();

        Serial.println("Game console started.");
        std::vector<DirectionalController *> controllers(1);
        controllers[0] = joystick_controller;

        std::vector<ActionController *> action_controllers(1);
        action_controllers[0] = keypad_controller;

        DelayProvider *delay_provider =
            new ArduinoDelay((void (*)(int))&rtos_delay);

        Platform platform = {.display = &display,
                             .directional_controllers = &controllers,
                             .action_controllers = &action_controllers,
                             .delay_provider = delay_provider,
                             .persistent_storage = &persistent_storage};

        select_game(&platform);
}

void setup(void)
{
        // Initialise serial port for debugging
        Serial.begin(115200);
        while (!Serial) {
                ; // wait for serial port to connect
        }

        // Set up controllers
        pinMode(STICK_BUTTON_PIN, INPUT);
        pinMode(LEFT_BUTTON_PIN, INPUT);
        pinMode(DOWN_BUTTON_PIN, INPUT);
        pinMode(UP_BUTTON_PIN, INPUT);
        pinMode(RIGHT_BUTTON_PIN, INPUT);

        // Initializes the source of randomness from the
        // noise present on the first digital pin
        initialize_randomness_seed(analogRead(0));

        xTaskCreate(wifi_connection_task, "WifiTask", 256, NULL, 2, NULL);
        auto result =
            xTaskCreate(main_task, "MicroBoxTask", 1024, NULL, 1, NULL);
        if (result != pdPASS) {
                Serial.println("ERROR: Game task creation failed!");
        }
        vTaskStartScheduler();
}

void loop(void)
{
        // Empty. Things are done in RTOS tasks.
}
