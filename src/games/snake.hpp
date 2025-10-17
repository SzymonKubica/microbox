#pragma once

#include "../common/platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "common_transitions.hpp"
#include "game_executor.hpp"

typedef struct SnakeConfiguration {
        int speed;
        bool accelerate;
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
