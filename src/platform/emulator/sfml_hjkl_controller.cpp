// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_hjkl_controller.hpp"
#include <SFML/Graphics.hpp>

bool SfmlHjklInputController::poll_for_input(Direction *input) {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {
    *input = Direction::LEFT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
    *input = Direction::RIGHT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
    *input = Direction::UP;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
    *input = Direction::DOWN;
    return true;
  }
  return false;
};

void SfmlHjklInputController::setup() {};
#endif
