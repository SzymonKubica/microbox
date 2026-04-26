#if defined(MICROBOX_2)
#include "microbox_v2.hpp"
#include "../drivers/display/lcd_display_2_4_inch.hpp"
#include "../drivers/input/adafruit_mini_gamepad.hpp"
#include "../boards/generic/time_provider.hpp"
#include "../boards/esp32/wifi_provider.hpp"
#include "../boards/esp32/http_client.hpp"
#include "../interface/controller.hpp"
#include "../../common/logging.hpp"
#include "Adafruit_seesaw.h"
#include "Arduino.h"
#include <EEPROM.h>

#define TAG "microbox_v2_platform_definition"

Adafruit_seesaw ss(&Wire);

Platform *initialize_platform()
{
        LcdDisplay *display = new LcdDisplay();
        MiniGamepadController *controller = new MiniGamepadController(&ss);
        std::vector<DirectionalController *> controllers{controller};
        std::vector<ActionController *> action_controllers{controller};
        TimeProvider *time_provider = new ArduinoTimeProvider();
        WifiProvider *wifi_provider = new Esp32WifiProvider();
        Esp32HttpClient *client = new Esp32HttpClient();

        /**
         * Note that we are not importing the esp32-specific persistent storage
         * directly here. This is because persistent storage relies on
         * generic store/retrieve functions that need to be defined in header
         * files. This means that we cannot have an 'abstract' interface class
         * that then gets overridden by the platform-specific implementations
         * because virtual functions cannot be templetized. Because of this, we
         * need to use a more elaborate pattern with 'AbstractPersistentStorage'
         * that uses the CRTP pattern to allow for switching of the
         * implementation details.
         *
         * Because of this, we have made an exception here, whereby the
         * PersistentStorage is the only platform component that gets resolved
         * based on the target device outside of target definition files like
         * this one. This is the only instance where this platform-specific
         * resolution 'leaks out' of the target definition file. Refer to
         * `interface/persistent_storage.hpp` to see how this resolution is
         * performed.
         */
        PersistentStorage *persistent_storage = new PersistentStorage();

        Platform platform = {.display = display,
                             .directional_controllers = controllers,
                             .action_controllers = action_controllers,
                             .time_provider = time_provider,
                             .persistent_storage = persistent_storage,
                             .wifi_provider = wifi_provider,
                             .client = client};

        return new Platform(platform);
}

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

bool setup_adafruit_seesaw_i2c_connection()
{

        LOG_INFO(TAG, "Setting up seesaw I2C interface...");

        if (!ss.begin(0x50)) {
                LOG_INFO(TAG, "ERROR! seesaw not found");
                return false;
        }
        LOG_INFO(TAG, "seesaw started");
        uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
        if (version != 5743) {
                LOG_INFO(TAG, "Wrong firmware loaded? ");
                LOG_INFO(TAG, version);
                return false;
        }

        LOG_INFO(TAG, "Found Product 5743");

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

        xTaskCreate(rgb_blink_task,            // function
                    "RGB diode blinking task", // name
                    2048,                      // stack size
                    NULL,                      // parameter
                    1,                         // priority
                    NULL                       // task handle
        );
}

size_t getArduinoLoopTaskStackSize() { return 32 * 1024; }
#endif
