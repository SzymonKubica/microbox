#pragma once
#include "../common/platform/interface/platform.hpp"
#include "../common/user_interface_customization.hpp"
#include "../common/configuration.hpp"

class GameExecutor
{
      public:
        virtual std::optional<UserAction>
        game_loop(Platform *p, UserInterfaceCustomization *customization) = 0;
};
