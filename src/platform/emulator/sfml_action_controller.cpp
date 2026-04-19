// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_action_controller.hpp"
#include <SFML/Graphics.hpp>

bool SfmlActionInputController::poll_for_input(Action *input) {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
    *input = Action::RED;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G)) {
    *input = Action::GREEN;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::B)) {
    *input = Action::BLUE;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y)) {
    *input = Action::YELLOW;
    return true;
  }
  return false;
};

void SfmlActionInputController::setup() {};
#endif
