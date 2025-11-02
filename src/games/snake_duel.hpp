#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

typedef struct SnakeDuelConfiguration {
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
         * Color of the snake that the keypad player owns.
         */
        Color secondary_player_color;
} SnakeConfiguration;

std::optional<UserAction>
collect_snake__duel_config(Platform *p, SnakeDuelConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class SnakeDuel : public GameExecutor
{
      public:
        virtual void
        game_loop(Platform *p,
                  UserInterfaceCustomization *customization) override;

        SnakeDuel() {}
};
