#include <cstring>

#include "../common/logging.hpp"
#include "../common/maths_utils.hpp"
#include "random_seed_picker.hpp"
#include "settings.hpp"
#include "game_menu.hpp"

#define TAG "random_seed_picker"

RandomSeedPickerConfiguration DEFAULT_RANDOM_SEED_PICKER_CONFIG = {
    .seed = 1234, .action = RandomSeedSelectorAction::Download};

const char *selector_action_to_str(RandomSeedSelectorAction action)
{
        switch (action) {
        case RandomSeedSelectorAction::Spin:
                return "Spin";
        case RandomSeedSelectorAction::Download:
                return "Download";
        case RandomSeedSelectorAction::Modify:
                return "Modify";
        default:
                return "UNKNOWN";
        }
}

RandomSeedSelectorAction selector_action_from_str(char *name)
{
        if (strcmp(name,
                   selector_action_to_str(RandomSeedSelectorAction::Spin)) == 0)
                return RandomSeedSelectorAction::Spin;
        if (strcmp(name, selector_action_to_str(
                             RandomSeedSelectorAction::Modify)) == 0)
                return RandomSeedSelectorAction::Modify;
        if (strcmp(name, selector_action_to_str(
                             RandomSeedSelectorAction::Download)) == 0)
                return RandomSeedSelectorAction::Download;
        // For now we fallback on this one as a default.
        return RandomSeedSelectorAction::Download;
}

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization);
void RandomSeedPicker::game_loop(Platform *p,
                                 UserInterfaceCustomization *customization)
{
        const char *help_text =
            "Select 'Modify' action and press next (red) to change the seed"
            "Select 'Download' to fetch a new seed from API (wifi connection "
            "required)."
            "Select 'Spin' to srand";

        bool exit_requested = false;
        while (!exit_requested) {
                switch (random_seed_picker_loop(p, customization)) {
                case UserAction::PlayAgain:
                        LOG_INFO(TAG, "Re-entering the main wifi app loop.");
                        continue;
                case UserAction::Exit:
                        exit_requested = true;
                        break;
                case UserAction::ShowHelp:
                        LOG_INFO(TAG,
                                 "User requsted help screen for wifi app.");
                        render_wrapped_help_text(p, customization, help_text);
                        wait_until_green_pressed(p);
                        break;
                }
        }
}

UserAction random_seed_picker_loop(Platform *p,
                                   UserInterfaceCustomization *customization)
{
        RandomSeedPickerConfiguration config;

        auto maybe_action =
            collect_random_seed_picker_config(p, &config, customization);
        if (maybe_action) {
                return maybe_action.value();
        }

        switch (config.action) {
        case RandomSeedSelectorAction::Spin:
                LOG_DEBUG(TAG, "Spin option selected");
                break;
        case RandomSeedSelectorAction::Download: {
                LOG_DEBUG(TAG, "Download option selected");
                const char *downloading_text = "Fetching new random seed...";
                render_wrapped_text(p, customization, downloading_text);

                const char *host = "www.randomnumberapi.com";
                std::string host_string(host);
                const int port = 80;
                auto resp =
                    p->client->get({.host = host_string, .port = port},
                                   "http://www.randomnumberapi.com/api/v1.0/"
                                   "random?min=0&max=10000&count=1");
                if (!resp.has_value()) {
                        LOG_DEBUG(TAG, "Did not receive a successful response "
                                       "from the API.");
                }
                int new_seed = 0;
                LOG_DEBUG(TAG, "Response from API: %s", resp.value().c_str());
                std::string response = resp.value();
                response.replace(response.find("["), 1, "");
                response.replace(response.find("]"), 1, "");
                response.erase(response.find_last_not_of(" \t\n\r\f\v") + 1);
                unsigned long seed = std::stoi(response);
                LOG_DEBUG(TAG, "Random seed from API: %d", seed);
                srand(seed);
                new_seed = seed;

                std::vector<int> offsets = get_settings_storage_offsets();
                int offset = offsets[RandomSeedPicker];
                config.seed = new_seed;
                p->persistent_storage->put(offset, config);

                char display_text_buffer[256];
                sprintf(display_text_buffer, "Fetched new randomness seed: %d",
                        new_seed);
                render_wrapped_help_text(p, customization, display_text_buffer);

                wait_until_green_pressed(p);
                break;
        }
        case RandomSeedSelectorAction::Modify:
                LOG_DEBUG(TAG, "Modify option selected");
                break;
        }

        return UserAction::PlayAgain;
}

RandomSeedPickerConfiguration *
load_initial_seed_picker_config(PersistentStorage *storage)
{

        int storage_offset = get_settings_storage_offsets()[RandomSeedPicker];

        RandomSeedPickerConfiguration config;
        LOG_DEBUG(TAG,
                  "Trying to load initial settings from the persistent "
                  "storage "
                  "at offset %d",
                  storage_offset);
        storage->get(storage_offset, config);

        RandomSeedPickerConfiguration *output =
            new RandomSeedPickerConfiguration();

        if (config.seed == 0) {
                LOG_DEBUG(TAG,
                          "The storage does not contain a valid "
                          "seed picker configuration, using default values.");
                memcpy(output, &DEFAULT_RANDOM_SEED_PICKER_CONFIG,
                       sizeof(RandomSeedPickerConfiguration));
                storage->put(storage_offset, DEFAULT_RANDOM_SEED_PICKER_CONFIG);

        } else {
                LOG_DEBUG(TAG, "Using configuration from persistent storage.");
                memcpy(output, &config, sizeof(RandomSeedPickerConfiguration));
        }

        LOG_DEBUG(TAG,
                  "Loaded random seed picker configuration: seed=%d, action=%d",
                  output->seed, output->action);

        return output;
}

Configuration *assemble_random_seed_picker_configuration(
    RandomSeedPickerConfiguration *initial_config);
void extract_seed_picker_config(
    RandomSeedPickerConfiguration *random_seed_picker_config,
    Configuration *config);

std::optional<UserAction>
collect_random_seed_picker_config(Platform *p,
                                  RandomSeedPickerConfiguration *game_config,
                                  UserInterfaceCustomization *customization)
{
        RandomSeedPickerConfiguration *initial_config =
            load_initial_seed_picker_config(p->persistent_storage);
        Configuration *config =
            assemble_random_seed_picker_configuration(initial_config);

        auto maybe_interrupt_action =
            collect_configuration(p, config, customization);
        if (maybe_interrupt_action) {
                return maybe_interrupt_action;
        }

        extract_seed_picker_config(game_config, config);
        free(initial_config);
        return std::nullopt;
}

Configuration *assemble_random_seed_picker_configuration(
    RandomSeedPickerConfiguration *initial_config)

{
        auto *seed = ConfigurationOption::of_integers(
            "Seed", {initial_config->seed}, initial_config->seed);

        auto available_actions = {
            selector_action_to_str(RandomSeedSelectorAction::Download),
            selector_action_to_str(RandomSeedSelectorAction::Modify),
            selector_action_to_str(RandomSeedSelectorAction::Spin)};

        auto *app_action = ConfigurationOption::of_strings(
            "Action", available_actions,
            selector_action_to_str(initial_config->action));

        auto options = {seed, app_action};

        return new Configuration("Seed Picker", options);
}

void extract_seed_picker_config(
    RandomSeedPickerConfiguration *random_seed_picker_config,
    Configuration *config)
{

        ConfigurationOption seed = *config->options[0];
        ConfigurationOption app_action = *config->options[1];

        random_seed_picker_config->seed = seed.get_curr_int_value();
        random_seed_picker_config->action =
            selector_action_from_str(app_action.get_current_str_value());
}
