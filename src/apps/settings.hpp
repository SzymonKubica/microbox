#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../menu.hpp"

std::vector<int> get_settings_storage_offsets();
int get_settings_storage_offset(Game game);

typedef struct SettingsConfiguration {
        ConfigurationHeader header;
        Game selected_game;
} SettingsConfiguration;

/**
 * This 'game' is a settings menu responsible for setting the default values of
 * all config options for all games. The idea is that this 'game' allows for
 * chosing the game for which we want to set the default settings, then renders
 * the game configuration menu and allows the user to set the defaults using the
 * same UI as they would use before starting the game. The chosen settings are
 * then saved in the persistent storage and used as the default values in the
 * future.
 */
class Settings : public ApplicationExecutor<SettingsConfiguration>
{
      public:
        UserAction app_loop(Platform *p,
                            UserInterfaceCustomization *customization,
                            const SettingsConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       SettingsConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;
};
