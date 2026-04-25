#if defined(MICROBOX_1)
#include "microbox_v1.hpp"
#include "../drivers/display/lcd_display_1_69_inch.hpp"
#include "../drivers/input/input_shield.hpp"
#include "../boards/generic/time_provider.hpp"
#include "../boards/arduino_r4_wifi/wifi_provider.hpp"
#include "../boards/arduino_r4_wifi/http_client.hpp"

Platform *initialize_platform()
{
        LcdDisplay_1_69 *display = new LcdDisplay_1_69();
        ArduinoInputShield *controller = new ArduinoInputShield();
        std::vector<DirectionalController *> controllers{controller};
        std::vector<ActionController *> action_controllers{controller};

        TimeProvider *time_provider = new ArduinoTimeProvider();
        WifiProvider *wifi_provider = new ArduinoWifiProvider();
        ArduinoHttpClient *client = new ArduinoHttpClient();

        /**
         * Here we are not using the platform-specific persistent storage
         * as is is resolved to the right implementation in
         * `interface/persistent_storage.hpp` Refer to `micrcobox_v2` to
         * understand why we are doing this.
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

void setup(Platform *platform)
{

        platform->display->setup();
        static_cast<ArduinoInputShield *>(platform->action_controllers[0])
            ->setup();
}
#endif
