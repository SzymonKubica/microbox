#include <optional>

#include "game_executor.hpp"
#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"
#include "common_transitions.hpp"

#define TAG "prototype"

UserAction snake_duel_loop(Platform *p,
                           UserInterfaceCustomization *customization);

const char *get_help_text();
const char *get_game_name();

inline void log_game_finished(const char *game_name);

inline bool close_window_requested(std::optional<UserAction> maybe_event);

std::optional<UserAction>
common_game_loop(GameExecutor *executor, Platform *p,
                 UserInterfaceCustomization *customization)
{

        while (true) {
                // Exit and Close actions are always propagated 1:1.
                switch (executor->game_loop(p, customization)) {
                case UserAction::Exit:
                        return UserAction::Exit;
                case UserAction::CloseWindow:
                        return UserAction::CloseWindow;
                case UserAction::PlayAgain: {
                        log_game_finished(executor->get_game_name());

                        Direction dir;
                        Action act;

                        auto maybe_event = pause_until_input(
                            p->directional_controllers, p->action_controllers,
                            &dir, &act, p->time_provider, p->display);

                        /*
                         * Here things get a bit complex on the emulator:
                         * The user might close the SFML window at any point
                         * during the runtime of the program. If that happens,
                         * we need to propagate this event all the way up to the
                         * emulator entrypoint and ensure that all resources
                         * get deallocated. The problem is that this event can
                         * happen literally at any time (expecially when the
                         * game is 'paused' waiting for input as above). This
                         * is solved by putting a call to `display.refresh()`
                         * inside of the function above. If a close window
                         * event is received, we need to propagate it further,
                         * hence the two return pathways out of this function.
                         */
                        if (close_window_requested(maybe_event)) {
                                return UserAction::CloseWindow;
                        }

                        if (act == Action::BLUE)
                                return UserAction::Exit;
                }
                case UserAction::ShowHelp: {
                        LOG_DEBUG(TAG, "User requested help screen");
                        render_wrapped_help_text(p, customization,
                                                 get_help_text());
                        // As above we might need handle window close events.
                        auto maybe_event = wait_until_green_pressed(p);
                        if (close_window_requested(maybe_event)) {
                                return UserAction::CloseWindow;
                        }
                        break;
                }
                }
        }
}

inline bool close_window_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event.has_value() &&
               maybe_event.value() == UserAction::CloseWindow;
}

inline void log_game_finished(const char *game_name)
{
        LOG_DEBUG(TAG, "%s game loop finished. Pausing for input ",
                  get_game_name());
}
const char *get_help_text()
{
        const char *help_text =
            "Use the joystick to control where the snake goes."
            "Consume apples to grow the snake. Avoid hitting the walls or "
            "snake's tail. Second player: use keypad to control the snake.";

        return help_text;
}

const char *get_game_name() { return "Snake Duel"; }
