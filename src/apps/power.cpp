#include <cassert>
#include <cstring>
#include "power.hpp"

// TODO: remove. This is here only for testing. Game code
// should not depend on the specifics of a particular display.
#if defined(EMULATOR)
#include "../platform/emulator/sfml_display.hpp"
#else
#include "../platform/drivers/display/lcd_display_2_4_inch.hpp"
#endif

#include "../common/configuration.hpp"

#define INPUT_REGISTERED_DELAY 150

#define TAG "power_management_app"

#if defined(WAVESHARE_2_4_INCH_LCD) // TODO: make this specific to the esp32
#include "esp_sleep.h"
#include "Arduino.h"
#endif

const char *PowerManagementApp::get_game_name() const { return "Power Off"; };
const char *PowerManagementApp::get_help_text() const
{
        return "Press any button to enter deep sleep. Press 'back' to cancel. "
               "Reset the console using the power button to wake up.";
};

UserAction
PowerManagementApp::app_loop(const Platform &p,
                             const UserInterfaceCustomization &customization,
                             const SleepConfiguration &config) const
{
#if defined(ARDUINO_ARCH_ESP32)
#define DEV_BL_PIN 4
        // We are entering deep sleep and shutting down the main CPU.
        // The intent is that resetting the console is the only way to
        // wake up now.
        p.display->sleep();
        gpio_hold_en((gpio_num_t)DEV_BL_PIN);
        gpio_deep_sleep_hold_en();
        esp_deep_sleep_start();
#endif
        // This will never be reached as we are entering the deep sleep above.
        return UserAction::Exit;
}

std::optional<UserAction> PowerManagementApp::collect_config(
    const Platform &p, const UserInterfaceCustomization &customization,
    SleepConfiguration &game_config) const
{
        render_wrapped_text(p, customization, get_help_text());
        Action act;
        wait_until_action_input(p, act);

        if (act == BACK_ACTION)
                return UserAction::Exit;
        return std::nullopt;
}

void PowerManagementApp::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{

        const auto &display = *platform.display;
        int available_height =
            display.get_height() - display.get_display_corner_radius();
        display.clear_region({0, available_height / 2},
                             {display.get_width(), available_height}, Black);
        const char *subtitle = "Power Off";
        render_menu_subtitle(
            display, Configuration(subtitle, {new ConfigurationOption()}),
            false, strlen(subtitle), customization);

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();

        // [BEGIN lopaka generated]
        tft.fillCircle(162, 137, 27, White);
        tft.fillCircle(162, 137, 27, White);
        tft.fillCircle(162, 137, 15, 0x0);
        tft.fillRect(158, 107, 9, 28, customization.accent_color);
        tft.fillRect(167, 100, 5, 39, 0x0);
        tft.fillCircle(162, 105, 4, customization.accent_color);
        tft.fillCircle(162, 134, 4, customization.accent_color);
        tft.fillRect(153, 100, 5, 41, 0x0);
        // [END lopaka generated]
}
