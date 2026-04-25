#include "microbox_v2.hpp"
#include "../drivers/display/lcd_display_2_4_inch.hpp"
#include "../drivers/input/adafruit_mini_gamepad.hpp"
#include "../boards/generic/time_provider.hpp"
#include "../boards/esp32/wifi_provider.hpp"
#include "../boards/esp32/http_client.hpp"
#include "../boards/esp32/persistent_storage.hpp"
#include "../interface/controller.hpp"
#include "Adafruit_seesaw.h"
#include "Arduino.h"
#include <EEPROM.h>

Adafruit_seesaw ss(&Wire);

Platform *initialize_platform()
{
        MiniGamepadController *adafruit_controller;

        MiniGamepadController *controller = new MiniGamepadController(&ss);
        TimeProvider *time_provider = new ArduinoTimeProvider();
        WifiProvider *wifi_provider = new Esp32WifiProvider{};
        Esp32HttpClient *client = new Esp32HttpClient();
        PersistentStorage *persistent_storage = new PersistentStorage();

        std::vector<DirectionalController *> controllers{controller};
        std::vector<ActionController *> action_controllers{controller};

        LcdDisplay *display = new LcdDisplay();

        Platform platform = {.display = display,
                             .directional_controllers = controllers,
                             .action_controllers = action_controllers,
                             .time_provider = time_provider,
                             .persistent_storage = persistent_storage,
                             .wifi_provider = wifi_provider,
                             .client = client};

        return new Platform(platform);
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
        pinMode(38, INPUT);

        return true;
}
void setup(Platform *platform)
{
        platform->display->setup();
        platform->persistent_storage->setup();
        setup_adafruit_seesaw_i2c_connection();
}
