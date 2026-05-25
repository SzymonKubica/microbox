#pragma once

#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../common/common_transitions.hpp"
#include "../application_executor.hpp"

struct SnakeConfiguration {
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
         * If true, users can pause the game by pressing the
         * yellow button.
         */
        bool allow_pause;
};

std::optional<UserAction>
collect_snake_config(Platform *p, SnakeConfiguration *game_config,
                     UserInterfaceCustomization *customization);

class SnakeGame : public ApplicationExecutor<SnakeConfiguration>,
                  public ThumbnailRenderer
{
      public:
        SnakeGame() {}

        UserAction app_loop(const Platform &p,
                            const UserInterfaceCustomization &customization,
                            const SnakeConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       SnakeConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;
        void render_thumbnail(
            const Platform &platform,
            const UserInterfaceCustomization &customization) override;
};
