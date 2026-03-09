#include <cassert>
#include "sleep.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "sleep_app"

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
#include "Arduino.h"
#endif

const char *SleepApp::get_game_name() { return "SleepApp"; };
const char *SleepApp::get_help_text()
{
        return "Press red (right) button to enter the sleep mode."
               "The screen will turn off."
               "Press any button to exit the sleep mode.";
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
#endif

        Action act;
        wait_until_action_input(p, &act);
        p->time_provider->delay_ms(INPUT_REGISTERED_DELAY);

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
                                    // battery driven deployment
        analogWrite(DEV_BL_PIN, 140);
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
