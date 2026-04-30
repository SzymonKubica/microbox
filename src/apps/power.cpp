#include <cassert>
#include "power.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "power_management_app"

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
#include "esp_sleep.h"
#include "Arduino.h"
#endif

const char *PowerManagementApp::get_game_name()
{
        return "PowerManagementApp";
};
const char *PowerManagementApp::get_help_text()
{
        return "Press any button to enter deep sleep. Press 'back' to cancel. "
               "Reset the console using the power button to wake up.";
};

void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score);

UserAction
PowerManagementApp::app_loop(Platform *p,
                             UserInterfaceCustomization *customization,
                             const SleepConfiguration &config)
{
#if defined(ARDUINO_ARCH_ESP32)
#define DEV_BL_PIN 4
        // We are entering deep sleep and shutting down the main CPU.
        // The intent is that resetting the console is the only way to
        // wake up now.
        p->display->sleep();
        gpio_hold_en((gpio_num_t)DEV_BL_PIN);
        gpio_deep_sleep_hold_en();
        esp_deep_sleep_start();
#endif
        // This will never be reached as we are entering the deep sleep above.
        return UserAction::Exit;
}

std::optional<UserAction>
PowerManagementApp::collect_config(Platform *p,
                                   UserInterfaceCustomization *customization,
                                   SleepConfiguration *game_config)
{
        render_wrapped_text(p, customization, get_help_text());
        Action act;
        wait_until_action_input(p, &act);

        if (act == BACK_ACTION)
                return UserAction::Exit;
        return std::nullopt;
}
