// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_asdf_controller.hpp"
#include <SFML/Graphics.hpp>

bool SfmlAsdfInputController::poll_for_input(Action *input)
{
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                *input = Action::BLUE;
                return true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                *input = Action::GREEN;
                return true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                *input = Action::YELLOW;
                return true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
                *input = Action::RED;
                return true;
        }
        return false;
};

void SfmlAsdfInputController::setup() {};
#endif
