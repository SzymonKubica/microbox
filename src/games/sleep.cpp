#include <cassert>
#include "sleep.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "sleep_app"

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
#include "esp_sleep.h"
#include "Arduino.h"
#endif

const char *SleepApp::get_game_name() { return "SleepApp"; };
const char *SleepApp::get_help_text()
{
        return "Press red (right) button to enter the sleep mode."
               "The screen will turn off."
               "Press and hold any button for 10s to exit the sleep mode.";
};

void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score);

UserAction SleepApp::app_loop(Platform *p,
                              UserInterfaceCustomization *customization,
                              const SleepConfiguration &config)
{

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
                                    // battery driven deployment
#define DEV_BL_PIN 4
        analogWrite(DEV_BL_PIN, 0);

        while (true) {
                // sleep for 5 seconds and then check for input, if registered
                // exit out of the loop.
                esp_sleep_enable_timer_wakeup(10 * 1000000);
                esp_light_sleep_start(); // CPU pauses here

                Action action;
                if (poll_action_input(p->action_controllers, &action)) {
                        break;
                }
        }

        // After we exit the loop, we turn the screen back on.
        analogWrite(DEV_BL_PIN, 140);
        // We wait for a bit so that the user has time to release the button
        // before going into another sleep session
        p->time_provider->delay_ms(1000);
#endif
        return UserAction::PlayAgain;
}

std::optional<UserAction>
SleepApp::collect_config(Platform *p, UserInterfaceCustomization *customization,
                         SleepConfiguration *game_config)
{
        render_wrapped_text(p, customization, get_help_text());
        game_config->sleep = true;
        Action act;
        wait_until_action_input(p, &act);
        return std::nullopt;
}
