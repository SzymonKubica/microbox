#ifdef EMULATOR
#include "emulator_config.h"

#include "src/common/platform/interface/platform.hpp"
#include "src/common/constants.hpp"
#include "src/common/platform/emulator/sfml_display.hpp"
#include "src/common/platform/emulator/emulated_wifi_provider.cpp"
#include "src/common/platform/emulator/emulator_http_client.hpp"
#include "src/common/platform/emulator/emulator_delay.cpp"
#include "src/common/platform/emulator/sfml_controller.hpp"
#include "src/common/platform/emulator/sfml_awsd_controller.hpp"
#include "src/common/platform/emulator/sfml_hjkl_controller.hpp"
#include "src/common/platform/emulator/sfml_action_controller.hpp"
#include "src/common/platform/emulator/persistent_storage.hpp"

#include "src/common/logging.hpp"

#include "src/games/game_menu.hpp"

#define TAG "emulator_entrypoint"

SfmlDisplay *display;
EmulatorDelay delay;
SfmlInputController controller;
SfmlAwsdInputController awsd_controller;
SfmlHjklInputController hjkl_controller;
SfmlActionInputController action_controller;
PersistentStorage persistent_storage;
WifiProvider *wifi_provider;
EmulatorHttpClient *client;

void print_version(char *argv[]);
int main(int argc, char *argv[])
{
        print_version(argv);

        LOG_DEBUG(TAG, "Emulator enabled!");

        sf::RenderWindow window(sf::VideoMode({DISPLAY_WIDTH, DISPLAY_HEIGHT}),
                                "game-console-emulator");

        // The problem with simply rendering to the window is that we would need
        // to redraw everything every frame. This is not the behaviour we want
        // as the arduino display doesn't work this way. In Arduino lcd, once
        // we draw something, it stays there until something is drawn on top of
        // it. We achieve this behaviour by using a RenderTexture. This texture
        // is then written into by the game engine and stores the drawn shapes
        // until something is drawn on top of it.
        sf::RenderTexture texture({DISPLAY_WIDTH, DISPLAY_HEIGHT});

        LOG_DEBUG(TAG, "Window rendered!");

        LOG_DEBUG(TAG, "Initializing the display...");
        display = new SfmlDisplay(&window, &texture);
        display->setup();
        LOG_DEBUG(TAG, "Display initialized!");

        controller = SfmlInputController{};
        awsd_controller = SfmlAwsdInputController{};
        hjkl_controller = SfmlHjklInputController{};
        action_controller = SfmlActionInputController{};

        persistent_storage = PersistentStorage{};

        std::vector<DirectionalController *> controllers = {
            &controller,
            &awsd_controller,
            &hjkl_controller,
        };

        std::vector<ActionController *> action_controllers = {
            &action_controller,
        };

        wifi_provider = new EmulatedWifiProvider();
        client = new EmulatorHttpClient();

        Platform platform = {.display = display,
                             .directional_controllers = &controllers,
                             .action_controllers = &action_controllers,
                             .delay_provider = &delay,
                             .persistent_storage = &persistent_storage,
                             .wifi_provider = wifi_provider,
                             .client = client};

        while (window.isOpen()) {
                LOG_DEBUG(TAG, "Entering game loop...");
                // We need to loop forever here as the game loop exits when the
                // game is over.
                while (true) {
                        auto maybe_action = select_game(&platform);
                        if (maybe_action.has_value() &&
                            maybe_action.value() == UserAction::CloseWindow) {
                                LOG_DEBUG(TAG, "User requested to close the "
                                               "window. Exiting...");
                                break;
                        }
                }
        }
        delete (EmulatedWifiProvider *)wifi_provider;
        delete (EmulatorHttpClient *)client;
}

void print_version(char *argv[])
{
        std::cout << argv[0] << "Version: " << EMULATOR_VERSION_MAJOR << "."
                  << EMULATOR_VERSION_MINOR << std::endl;
}
#endif // EMULATOR
