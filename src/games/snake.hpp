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
         * If true, the game engine will wait for an extra tick before ending
         * the game when the player is about to crash into a wall or snake's tail.
         */
        bool allow_grace;
        /**
         * If true, the snake snake will leave excrements after eating an apple
         * Right now this is only a visual effect, in the future we might
         * add a game mechanic tied to that;
         */
        bool enable_poop;
        /**
         * If true, users can pause the game by pressing the
         * yellow button.
         */
        bool allow_pause;
} SnakeConfiguration;

std::optional<UserAction>
collect_snake_config(Platform *p, SnakeConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class SnakeGame : public GameExecutor
{
      public:
        virtual std::optional<UserAction>
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        SnakeGame() {}
};
