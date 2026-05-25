#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"
#include <optional>

struct GameOfLifeConfiguration {
        ConfigurationHeader header;
        bool prepopulate_grid;
        bool use_toroidal_array;
        /**
         * Simulation steps taken per second
         */
        int simulation_speed;
        /**
         * Controls how many steps the user is allowed to rewind the simulation
         */
        int rewind_buffer_size;
};

class GameOfLife : public ApplicationExecutor<GameOfLifeConfiguration>,
                   public ThumbnailRenderer
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const GameOfLifeConfiguration &config) const override;
        std::optional<UserAction>
        collect_config(const Platform &p,
                       const UserInterfaceCustomization &customization,
                       GameOfLifeConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        void render_thumbnail(
            const Platform &platform,
            const UserInterfaceCustomization &customization) override;

        GameOfLife() {}
};
