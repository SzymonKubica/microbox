#include <cassert>
#include "brightness.hpp"

#include "../common/configuration.hpp"
#include "../common/grid.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "sleep_app"

#if defined(WAVESHARE_2_4_INCH_LCD)
#include "Arduino.h"
#endif

const char *BrightnessApp::get_game_name() { return "BrightnessApp"; };
const char *BrightnessApp::get_help_text()
{
        return "Press right to adjust the brightness.";
};

void update_score(Platform *p, SquareCellGridDimensions *dimensions,
                  int score_text_end_location, int score);

UserAction BrightnessApp::app_loop(Platform *p,
                                   UserInterfaceCustomization *customization,
                                   const BrightnessConfiguration &config)
{

        char *brightness_str;
        auto maybe_interrupt = collect_string_input(
            p, customization, "Enter brightness 0-255",
            &brightness_str);
        if (maybe_interrupt.has_value()) {
                return maybe_interrupt.value();
        }
        int brightness = atoi(brightness_str);
        assert(0 < brightness && brightness <= 255);
#if defined(WAVESHARE_2_4_INCH_LCD)
#define DEV_BL_PIN 4
        analogWrite(DEV_BL_PIN, brightness);
#endif
        return UserAction::Exit;
}

std::optional<UserAction>
BrightnessApp::collect_config(Platform *p,
                              UserInterfaceCustomization *customization,
                              BrightnessConfiguration *game_config)
{
        render_wrapped_text(p, customization, get_help_text());
        Action act;
        wait_until_action_input(p, &act);
        return std::nullopt;
}
