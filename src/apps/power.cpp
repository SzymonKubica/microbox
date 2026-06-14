#include <cassert>
#include <cstring>
#include "power.hpp"

#include "../common/configuration.hpp"

#define TAG "power_management_app"

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
        p.display->sleep();
        p.power_manager->enter_deep_sleep();

        // This will never be reached as we are entering the deep sleep above
        // and so the only way to wake up is to reset the console.
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
        clear_half_display_and_render_subtitle(platform, customization,
                                               subtitle);

        TftCompatibleDisplay &tft =
            *platform.display->cast_into_tft_compatible();

        // [BEGIN lopaka generated]
        tft.fillCircle(160, 137, 27, White);
        tft.fillCircle(160, 137, 15, 0x0);
        tft.fillRect(156, 107, 9, 28, customization.accent_color);
        tft.fillRect(165, 100, 5, 39, 0x0);
        tft.fillCircle(160, 105, 4, customization.accent_color);
        tft.fillCircle(160, 134, 4, customization.accent_color);
        tft.fillRect(151, 100, 5, 41, 0x0);
        // [END lopaka generated]
}
