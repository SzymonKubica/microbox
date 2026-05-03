#include "../controller.hpp"

/**
 * Checks if any of the controllers has recorded user input. If so, the input
 * direction will be written into the `registered_dir` output parameter.
 */
std::optional<Direction>
poll_directional_input(const std::vector<DirectionalController *> &controllers)
{
        // If multiple controllers register input, the first one takes
        // precedence.
        for (DirectionalController *controller : controllers) {
                auto maybe_value = controller->poll_for_direction();
                if (maybe_value.has_value())
                        return maybe_value;
        }
        return std::nullopt;
}

std::optional<Action>
poll_action_input(const std::vector<ActionController *> &controllers)
{
        // If multiple controllers register input, the first one takes
        // precedence.
        for (ActionController *controller : controllers) {
                auto maybe_value = controller->poll_for_action();
                if (maybe_value.has_value())
                        return maybe_value;
        }
        return std::nullopt;
}
