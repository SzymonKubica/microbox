#pragma once
#include "application_executor.hpp"
#include "common_transitions.hpp"
#include "../common/configuration.hpp"

enum class RandomSeedSelectorAction : uint8_t { Spin, Download, Modify };

const char *selector_action_to_str(RandomSeedSelectorAction action);
RandomSeedSelectorAction selector_action_from_str(char *string);

void enter_random_seed_picker_loop(Platform *platform,
                                   UserInterfaceCustomization *customization);

typedef struct RandomSeedPickerConfiguration {
        ConfigurationHeader header;
        int seed;
        RandomSeedSelectorAction action;
} RandomSeedPickerConfiguration;

/**
 * Collects the game of life configuration from the user.
 *
 * This is exposed publicly so that the default game configuration saving module
 * can call it, get the new default setttings and save them in the persistent
 * storage.
 */
std::optional<UserAction>
collect_random_seed_picker_config(Platform *p,
                                  RandomSeedPickerConfiguration *game_config,
                                  UserInterfaceCustomization *customization);

class RandomSeedPicker
    : public ApplicationExecutor<RandomSeedPickerConfiguration>
{
      public:
        UserAction
        app_loop(Platform *p, UserInterfaceCustomization *customization,
                 const RandomSeedPickerConfiguration &config) override;
        std::optional<UserAction>
        collect_config(Platform *p, UserInterfaceCustomization *customization,
                       RandomSeedPickerConfiguration *game_config) override;
        const char *get_game_name() override;
        const char *get_help_text() override;

        RandomSeedPicker() {}
};
