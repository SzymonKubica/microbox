// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_asdf_controller.hpp"
#include <SFML/Graphics.hpp>

std::optional<Action> SfmlAsdfInputController::poll_for_action()
{
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                return Action::BLUE;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                return Action::GREEN;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                return Action::YELLOW;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
                return Action::RED;
        }
        return std::nullopt;
};

void SfmlAsdfInputController::setup() {};
#endif
