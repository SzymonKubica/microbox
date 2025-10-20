#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

typedef struct SnakeConfiguration {
        /**
         * Speed of the snake in cells travelled per second.
         */
        int speed;
        /**
         * [UNSUPPORTED YET] If true, the snake speeds up the larger it gets.
         */
        bool accelerate;
        /**
         * [UNSUPPORTED YET] If true, users can pause the game by pressing the
         * yellow button.
         */
        bool allow_pause;
} SnakeConfiguration;

std::optional<UserAction>
collect_snake_config(Platform *p, SnakeConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class Snake : public GameExecutor
{
      public:
        virtual void
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        Snake() {}
};
