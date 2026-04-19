#include "../common/configuration.hpp"
#include "../platform/interface/display.hpp"
#include "../platform/interface/controller.hpp"
#include "../platform/interface/time_provider.hpp"
#include "../common/user_interface_customization.hpp"

void display_game_over(Display *display,
                       UserInterfaceCustomization *customization);
void display_game_won(Display *display,
                      UserInterfaceCustomization *customization);

std::optional<UserAction> pause_until_any_directional_input(
    std::vector<DirectionalController *> *controllers,
    TimeProvider *delay_provider, Display *display);
/**
 *
 */
std::optional<UserAction> pause_until_input(std::vector<DirectionalController *> *controllers,
                       std::vector<ActionController *> *action_controllers,
                       Direction *direction, Action *action,
                       TimeProvider *delay_provider, Display *display);
