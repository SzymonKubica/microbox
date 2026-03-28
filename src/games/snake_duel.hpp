#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "application_executor.hpp"

typedef struct SnakeDuelConfiguration{
        ConfigurationHeader header;
        /**
         * Speed of the snake in cells travelled per second.
         */
        int speed;
        /**
         * If true, the game engine will wait for an extra tick before ending
         * the game when the player is about to crash into a wall or snake's
         * tail.
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
        bool enable_ai = true;
} SnakeDuelConfiguration;

std::optional<UserAction>
collect_snake_duel_config(Platform *p, SnakeDuelConfiguration *game_config,
                          UserInterfaceCustomization *customization);

class SnakeDuel : public ApplicationExecutor<SnakeDuelConfiguration>
{
      public:
        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const SnakeDuelConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       SnakeDuelConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;

        SnakeDuel() {}
};
