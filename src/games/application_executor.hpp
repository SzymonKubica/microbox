#pragma once
#include <cassert>
#include "../common/platform/interface/platform.hpp"
#include "../common/user_interface_customization.hpp"
#include "../common/configuration.hpp"

#include "game_menu.hpp"
#include "../common/configuration.hpp"
#include "../common/constants.hpp"
#include "../common/grid.hpp"
#include "../common/logging.hpp"
#include "common_transitions.hpp"

#define EXECUTOR_TAG "executor"

/**
 * All applications that run on MicroBox should instantiate this template.
 * The mental model of all applications consists of two steps:
 * 1. Load saved preset configuration and allow the user to modify this
 * configuration:
 *    - this is required by all games where the user gets to specify all game
 *    input parameters pior to playing the game (e.g. difficulty, speed, ...)
 *    - the reason we require this as part of the interface is to allow for
 *    reusing the UI that renders the config options and provide a reusable
 *    templat that handles the common state transistions driven by the user
 *    input in `execute_app`.
 * 2. Execute a single run of the application/game:
 *    - this is the actual interesting part of each executor, for games this
 *    is the game loop that provides a single playthrough and for other
 *    applictions it is the actual work that the application performs (e.g.
 *    getting wifi SSID and password settings and connecting to the wifi)
 */
template <typename ConfigStruct> class ApplicationExecutor
{
      public:
        /**
         * The main loop of the application/game representing a single
         * application work / playthrough.
         */
        virtual UserAction app_loop(Platform *p,
                                    UserInterfaceCustomization *customization,
                                    const ConfigStruct &config) = 0;
        /**
         * Configuration collecting method, this is the first step of each app.
         * Note how this is parameterized with the `ConfigStruct` template type
         * parameter, this is important to allow the apps to collect their
         * specific config.
         */
        virtual std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       ConfigStruct *game_config) = 0;

        /**
         * Static text that will be rendered when the user requests the help
         * screen.
         */
        virtual const char *get_help_text() = 0;

        /**
         * This is required to customize the logs in `execute_app` function
         * to customize the logs to align with the application.
         */
        virtual const char *get_game_name() = 0;
        virtual ~ApplicationExecutor() {}
};

inline void log_game_finished(const char *app_name);
inline void log_help_requested(const char *app_name);
inline void log_exit_requested(const char *app_name);
inline bool close_window_requested(std::optional<UserAction> maybe_event);
inline bool exit_requested(std::optional<UserAction> maybe_event);
inline bool help_requested(std::optional<UserAction> maybe_event);

template <typename ConfigStruct>
std::optional<UserAction>
execute_app(ApplicationExecutor<ConfigStruct> *executor, Platform *p,
            UserInterfaceCustomization *customization)
{

        while (true) {
                // Before each game/application we allow the user to specify its
                // configuration and possibly review the help message.
                ConfigStruct config;
                auto maybe_event =
                    executor->collect_config(p, customization, &config);

                if (exit_requested(maybe_event)) {
                        log_exit_requested(executor->get_game_name());
                        return UserAction::Exit;
                }

                if (help_requested(maybe_event)) {
                        log_help_requested(executor->get_game_name());
                        render_wrapped_help_text(p, customization,
                                                 executor->get_help_text());
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

                auto action = executor->app_loop(p, customization, config);

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

inline bool maybe_event_matches(const std::optional<UserAction> &maybe_event,
                                UserAction expected_action)
{

        return maybe_event.has_value() &&
               maybe_event.value() == expected_action;
}
inline bool close_window_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event_matches(maybe_event, UserAction::CloseWindow);
}

inline bool exit_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event_matches(maybe_event, UserAction::Exit);
}

inline bool help_requested(std::optional<UserAction> maybe_event)
{
        return maybe_event_matches(maybe_event, UserAction::ShowHelp);
}

inline void log_game_finished(const char *game_name)
{
        LOG_DEBUG(EXECUTOR_TAG, "%s game loop finished. Pausing for input ",
                  game_name);
}

inline void log_help_requested(const char *app_name)
{
        LOG_DEBUG(EXECUTOR_TAG, "User requested help screen for %s", app_name);
}

inline void log_exit_requested(const char *app_name)
{
        LOG_DEBUG(EXECUTOR_TAG, "User requested to exit from %s", app_name);
}
