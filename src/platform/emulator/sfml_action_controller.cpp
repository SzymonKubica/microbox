// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_action_controller.hpp"
#include <SFML/Graphics.hpp>
#include <optional>

std::optional<Action> SfmlActionInputController::poll_for_action()
{
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
                return Action::RED;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G)) {
                return Action::GREEN;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B)) {
                return Action::BLUE;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y)) {
                return Action::YELLOW;
        }
        return std::nullopt;
};

void SfmlActionInputController::setup() {};
#endif
