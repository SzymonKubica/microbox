#include "common_transitions.hpp"
#include "../common/constants.hpp"

void display_input_clafification(Display *display)
{

        {
                const char *msg = "Press blue to exit.";

                int height = display->get_height();
                int width = display->get_width();
                int x_pos = (width - strlen(msg) * FONT_WIDTH) / 2;
                int y_pos = (height - FONT_SIZE) / 2 + 2 * FONT_SIZE;
                Point text_position = {.x = x_pos, .y = y_pos};

                display->draw_string(text_position, (char *)msg, Size16, Black,
                                     White);
        }

        {
                const char *msg = "Tilt stick to try again.";

                int height = display->get_height();
                int width = display->get_width();
                int x_pos = (width - strlen(msg) * FONT_WIDTH) / 2;
                int y_pos = (height - FONT_SIZE) / 2 + 3 * FONT_SIZE;
                Point text_position = {.x = x_pos, .y = y_pos};

                display->draw_string(text_position, (char *)msg, Size16, Black,
                                     White);
        }
}

void display_game_over(Display *display,
                       UserInterfaceCustomization *customization)
{
        if (customization->rendering_mode == Detailed) {
                display->draw_rounded_border(Red);
        } else {
                // In the minimialistic UI mode we only clear the screen.
                display->clear(Black);
        }

        const char *msg = "Game Over";

        int height = display->get_height();
        int width = display->get_width();
        int x_pos = (width - strlen(msg) * FONT_WIDTH) / 2;
        int y_pos = (height - FONT_SIZE) / 2;

        Point text_position = {.x = x_pos, .y = y_pos};

        display->draw_string(text_position, (char *)msg, Size16, Black, Red);
        display_input_clafification(display);
}

void display_game_won(Display *display,
                      UserInterfaceCustomization *customization)
{
        if (customization->rendering_mode == Detailed) {
                display->draw_rounded_border(Green);
        } else {
                // In the minimialistic UI mode we only clear the screen.
                display->clear(Black);
        }

        const char *msg = "You Won!";

        int height = display->get_height();
        int width = display->get_width();
        int x_pos = (width - strlen(msg) * FONT_WIDTH) / 2;
        int y_pos = (height - FONT_SIZE) / 2;
        Point text_position = {.x = x_pos, .y = y_pos};

        display->draw_string(text_position, (char *)msg, Size16, Black, Green);

        display_input_clafification(display);
}

std::optional<UserAction> pause_until_any_directional_input(
    std::vector<DirectionalController *> *controllers,
    DelayProvider *delay_provider, Display *display)
{
        Direction dir;
        while (!poll_directional_input(controllers, &dir)) {
                delay_provider->delay_ms(INPUT_POLLING_DELAY);
                // On the target device this is a no-op, but on the SFML display
                // this ensures that we poll for events while waiting for input
                // and so if the user tries to close the window, it will get
                // closed properly.
                if (!display->refresh()) {
                        return UserAction::CloseWindow;
                }
        };
        return std::nullopt;
}

std::optional<UserAction>
pause_until_input(std::vector<DirectionalController *> *controllers,
                  std::vector<ActionController *> *action_controllers,
                  Direction *direction, Action *action,
                  DelayProvider *delay_provider, Display *display)
{
        while (!poll_directional_input(controllers, direction) &&
               !poll_action_input(action_controllers, action)) {
                delay_provider->delay_ms(INPUT_POLLING_DELAY);
                // On the target device this is a no-op, but on the SFML display
                // this ensures that we poll for events while waiting for input
                // and so if the user tries to close the window, it will get
                // closed properly.
                if (!display->refresh()) {
                        return UserAction::CloseWindow;
                }
        };
        return std::nullopt;
}
