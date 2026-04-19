// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_controller.hpp"
#include <SFML/Graphics.hpp>

bool SfmlInputController::poll_for_input(Direction *input) {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
    *input = Direction::LEFT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
    *input = Direction::RIGHT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
    *input = Direction::UP;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
    *input = Direction::DOWN;
    return true;
  }
  return false;
};

void SfmlInputController::setup() {};
#endif
