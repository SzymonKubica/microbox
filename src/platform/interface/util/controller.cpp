#include "../controller.hpp"

/**
 * Checks if any of the controllers has recorded user input. If so, the input
 * direction will be written into the `registered_dir` output parameter.
 */
bool poll_directional_input(const std::vector<DirectionalController *> &controllers,
                            Direction *registered_dir)
{
        bool input_registered = false;
        // If multiple  controllers register input, the last one takes
        // precedence as it's registered direction will override what was set by
        // the previous controllers.
        for (DirectionalController *controller : controllers) {
                input_registered |= controller->poll_for_input(registered_dir);
        }
        return input_registered;
}

bool poll_action_input(const std::vector<ActionController *> &controllers,
                       Action *registered_action)
{
        bool input_registered = false;
        // If multiple  controllers register input, the last one takes
        // precedence as it's registered action will override what was set by
        // the previous controllers.
        for (ActionController *controller : controllers) {
                input_registered |=
                    controller->poll_for_input(registered_action);
        }
        return input_registered;
}
