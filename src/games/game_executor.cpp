#include <optional>

#include "game_executor.hpp"
#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"
#include "common_transitions.hpp"

#define TAG "prototype"

inline void log_game_finished(const char *game_name);
inline bool close_window_requested(std::optional<UserAction> maybe_event);
inline bool show_help_requested(std::optional<UserAction> maybe_event);

template <typename ConfigStruct>
std::optional<UserAction>
common_game_loop(GameExecutor<ConfigStruct> *executor, Platform *p,
                 UserInterfaceCustomization *customization)
{

        while (true) {
                // Before each game we allow the user to specify its
                // configuration and possibly review the help message.
                ConfigStruct config;
                auto maybe_event =
                    executor.collect_config(p, customization, &config);

                if (show_help_requested(maybe_event)) {
                        LOG_DEBUG(TAG, "User requested help screen");
                        render_wrapped_help_text(p, customization,
                                                 executor.get_help_text());
                        auto maybe_event = wait_until_green_pressed(p);

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
                        // Iterate again to render config menu again after help
                        // window is dismissed.
                        continue;
                }

                auto action = executor->game_loop(p, customization);

                assert(action != UserAction::ShowHelp &&
                       "Showing game help is only supported in the game "
                       "configuration menu.");

                /*
                 * The assumption here is that once the game is over the user
                 * might want to see the final screen of the game (e.g. the
                 * dead snake or a solved sudoku congratulations message).
                 * Because of this, when a 'PlayAgain' action is recorded,
                 * we first wait for any input and then continue to the next
                 * iteration of the loop. If the user presses the Exit button,
                 * we exit.
                 */
                if (action == UserAction::PlayAgain) {
                        log_game_finished(executor->get_game_name());

                        Direction dir;
                        Action act;

                        auto maybe_event = pause_until_input(
                            p->directional_controllers, p->action_controllers,
                            &dir, &act, p->time_provider, p->display);

                        if (close_window_requested(maybe_event)) {
                                return UserAction::CloseWindow;
                        }

                        if (act == Action::BLUE)
                                return UserAction::Exit;
                        continue;
                }

                // All other actions (exit/close window) are propagated
                // 1:1 upwards.
                return action;
        }
}

inline bool close_window_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event.has_value() &&
               maybe_event.value() == UserAction::CloseWindow;
}

inline bool help_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event.has_value() &&
               maybe_event.value() == UserAction::ShowHelp;
}

inline void log_game_finished(const char *game_name)
{
        LOG_DEBUG(TAG, "%s game loop finished. Pausing for input ", game_name);
}
