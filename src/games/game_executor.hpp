#pragma once
#include "../common/platform/interface/platform.hpp"
#include "../common/user_interface_customization.hpp"
#include "../common/configuration.hpp"

template <typename ConfigStruct> class GameExecutor
{
      public:
        /**
         * The main loop of the game representing a single playthrough.
         */
        virtual UserAction game_loop(Platform *p,
                                     UserInterfaceCustomization *customization,
                                     const ConfigStruct &config) = 0;
        /**
         * Configuration collecting method, this is the first step of each game.
         */
        virtual std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       ConfigStruct *game_config);

        virtual const char *get_game_name() = 0;
        /**
         * Static text that will be rendered when the user requests the help
         * screen.
         */
        virtual const char *get_help_text() = 0;
        virtual ~GameExecutor() {}
};

template <typename ConfigStruct>
std::optional<UserAction>
common_game_loop(GameExecutor<ConfigStruct> *executor, Platform *p,
                 UserInterfaceCustomization *customization);
