#pragma once
#include "../common/platform/interface/platform.hpp"
#include "../common/user_interface_customization.hpp"
#include "../common/configuration.hpp"

class GameExecutor
{
      public:
        virtual UserAction
        game_loop(Platform *p, UserInterfaceCustomization *customization) = 0;
        virtual const char *get_game_name() = 0;
        virtual const char *get_help_text() = 0;
        virtual ~GameExecutor() {}
};
