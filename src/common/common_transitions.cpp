#include <cstring>
#include "../common/common_transitions.hpp"
#include "constants.hpp"

void display_input_clafification(const Display &display)
{
        auto [fw, fh] = display.get_font_configuration().font_dimensions;
        {
                const char *msg = "Press 'left' to exit.";

                int height = display.get_height();
                int width = display.get_width();
                int x_pos = (width - strlen(msg) * fw) / 2;
                int y_pos = (height - fh) / 2 + 2 * fh;
                Point text_position = {.x = x_pos, .y = y_pos};

                display.draw_string(text_position, (char *)msg, Size16, Black,
                                    White);
        }

        {
                const char *msg = "Tilt stick to try again.";

                int height = display.get_height();
                int width = display.get_width();
                int x_pos = (width - strlen(msg) * fw) / 2;
                int y_pos = (height - fh) / 2 + 3 * fh;
                Point text_position = {.x = x_pos, .y = y_pos};

                display.draw_string(text_position, (char *)msg, Size16, Black,
                                    White);
        }
}

void display_game_over(const Display &display,
                       const UserInterfaceCustomization &customization)
{
        auto [fw, fh] = display.get_font_configuration().font_dimensions;
        if (customization.rendering_mode == Detailed) {
                display.draw_rounded_border(Red);
                display.clear(Black);
        } else {
                // In the minimialistic UI mode we only clear the screen.
                display.clear(Black);
        }

        const char *msg = "Game Over";

        int height = display.get_height();
        int width = display.get_width();
        int x_pos = (width - strlen(msg) * fw) / 2;
        int y_pos = (height - fh) / 2;

        Point text_position = {.x = x_pos, .y = y_pos};

        display.draw_string(text_position, (char *)msg, Size16, Black, Red);
        display_input_clafification(display);
}

void display_game_won(const Display &display,
                      const UserInterfaceCustomization &customization)
{
        auto [fw, fh] = display.get_font_configuration().font_dimensions;
        if (customization.rendering_mode == Detailed) {
                display.draw_rounded_border(Green);
        } else {
                // In the minimialistic UI mode we only clear the screen.
                display.clear(Black);
        }

        const char *msg = "You Won!";

        int height = display.get_height();
        int width = display.get_width();
        int x_pos = (width - strlen(msg) * fw) / 2;
        int y_pos = (height - fh) / 2;
        Point text_position = {.x = x_pos, .y = y_pos};

        display.draw_string(text_position, (char *)msg, Size16, Black, Green);

        display_input_clafification(display);
}

std::optional<UserAction> pause_until_any_directional_input(
    const std::vector<DirectionalController *> &controllers,
    const TimeProvider &delay_provider, const Display &display)
{
        while (!poll_directional_input(controllers).has_value()) {
                delay_provider.delay_ms(INPUT_POLLING_DELAY);
                // On the target device this is a no-op, but on the SFML display
                // this ensures that we poll for events while waiting for input
                // and so if the user tries to close the window, it will get
                // closed properly.
                if (!display.refresh()) {
                        return UserAction::CloseWindow;
                }
        };
        return std::nullopt;
}

std::optional<UserAction>
pause_until_input(const std::vector<DirectionalController *> &controllers,
                  const std::vector<ActionController *> &action_controllers,
                  Direction *direction, Action *action,
                  const TimeProvider &delay_provider, const Display &display)
{

        while (!poll_directional_input(controllers).has_value() &&
               !poll_action_input(action_controllers).has_value()) {
                delay_provider.delay_ms(INPUT_POLLING_DELAY);
                // On the target device this is a no-op, but on the SFML display
                // this ensures that we poll for events while waiting for input
                // and so if the user tries to close the window, it will get
                // closed properly.
                if (!display.refresh()) {
                        return UserAction::CloseWindow;
                }
        };
        return std::nullopt;
}
