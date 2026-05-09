#pragma once
#include "../application_executor.hpp"
#include "../common/common_transitions.hpp"
#include "../common/configuration.hpp"

enum class RandomSeedSelectorAction : uint8_t { Spin, Download, Modify };

const char *selector_action_to_str(RandomSeedSelectorAction action);
RandomSeedSelectorAction selector_action_from_str(char *string);

struct RandomSeedPickerConfiguration {
        ConfigurationHeader header;
        int seed;
        RandomSeedSelectorAction action;
};

class RandomSeedPicker
    : public ApplicationExecutor<RandomSeedPickerConfiguration>
{
      public:
        UserAction
        app_loop(const Platform &p,
                 const UserInterfaceCustomization &customization,
                 const RandomSeedPickerConfiguration &config) const override;
        std::optional<UserAction> collect_config(
            const Platform &p, const UserInterfaceCustomization &customization,
            RandomSeedPickerConfiguration &game_config) const override;
        const char *get_game_name() const override;
        const char *get_help_text() const override;

        RandomSeedPicker() {}
};
