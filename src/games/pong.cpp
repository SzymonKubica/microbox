#include <stdbool.h>
#include <string.h>
#include <cstring>
#include <string>
#include <cassert>
#include "pong.hpp"

#include "../common/logging.hpp"
#include "../common/constants.hpp"
#include "../platform/interface/display.hpp"
#include "../platform/interface/platform.hpp"
#include "../common/configuration.hpp"

#include "../menu.hpp"
#include "../common/common_transitions.hpp"
#include "../apps/settings.hpp"

#define TAG "Pong"

PongConfiguration DEFAULT_PONG_GAME_CONFIG = {
    .header = ConfigurationHeader(),
};

const char *Pong::get_game_name() const { return "Pong"; }
const char *Pong::get_help_text() const { return "TODO"; }

UserAction Pong::app_loop(const Platform &p,
                          const UserInterfaceCustomization &customization,
                          const PongConfiguration &config) const
{
        return UserAction::PlayAgain;
}

PongConfiguration *load_initial_pong_config(const PersistentStorage &storage)
{
        int storage_offset = get_settings_storage_offset(Game::Pong);

        PongConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent storage "
                  "at offset %d",
                  storage_offset);
        storage.get(storage_offset, config);

        PongConfiguration *output = new PongConfiguration();

        if (!config.header.validate_against(DEFAULT_PONG_GAME_CONFIG)) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "pong game configuration, using default values.");
                memcpy(output, &DEFAULT_PONG_GAME_CONFIG,
                       sizeof(PongConfiguration));
                storage.put(storage_offset, DEFAULT_PONG_GAME_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(PongConfiguration));
        }

        // TODO: log loaded values here

        return output;
}

/**
 * Assembles the generic configuration struct that is needed to collect
 * user defined game configuration for pong. Note that this is a
 * declarative way of defining what can be configured and the UI code
 * then dynamically renders selectors and handles switching between
 * option values.
 *
 * WARNING: This is tightly coupled with the
 * `extract_game_config` function. If you change the
 * structure of this config, make sure to make a corresponding update to
 * that function below to ensure that the specific game config can be
 * successfully extracted from the generic config struct.
 */
Configuration *assemble_pong_configuration(PersistentStorage *storage,
                                           PongConfiguration *initial_config)
{

        // TODO: assemble configuration values here (example below)
        //// Initialize the first config option: game gridsize
        // auto *grid_size = ConfigurationOption::of_integers(
        //     "Grid size", {3, 4, 5}, initial_config->grid_size);

        // auto *game_target = ConfigurationOption::of_integers(
        //     "Game target", {128, 256, 512, 1024, 2048, 4096},
        //     initial_config->target_max_tile);

        std::vector<ConfigurationOption *> options = {};

        return new Configuration("Pong", options);
}
void extract_game_config(PongConfiguration &game_config,
                         const PongConfiguration &initial_config,
                         const Configuration &config)
{

        // TODO: unwrap config values here (see example below).
        //// Grid size is the first config option in the game struct
        //// above.
        // ConfigurationOption grid_size = *config.options[0];
        //// Game target is the second config option above.
        // ConfigurationOption game_target = *config.options[1];

        // game_config.grid_size = grid_size.get_curr_int_value();
        // game_config.target_max_tile = game_target.get_curr_int_value();
        // game_config.is_game_in_progress = initial_config.is_game_in_progress;
        // game_config.saved_grid_size = initial_config.saved_grid_size;
        // game_config.saved_target_max_tile =
        //     initial_config.saved_target_max_tile;
        // memcpy(&game_config.saved_grid, &initial_config.saved_grid,
        //        sizeof(initial_config.saved_grid));
}

std::optional<UserAction>
Pong::collect_config(const Platform &p,
                     const UserInterfaceCustomization &customization,
                     PongConfiguration &game_config) const
{
        auto initial_cfg = std::unique_ptr<PongConfiguration>(
            load_initial_pong_config(*p.persistent_storage));
        auto cfg = std::unique_ptr<Configuration>(assemble_pong_configuration(
            p.persistent_storage, initial_cfg.get()));

        auto interrupt = collect_configuration(p, *cfg, customization);
        if (interrupt)
                return interrupt;
        extract_game_config(game_config, *initial_cfg, *cfg);
        return std::nullopt;
}
