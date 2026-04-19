// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_awsd_controller.hpp"
#include <SFML/Graphics.hpp>

bool SfmlAwsdInputController::poll_for_input(Direction *input) {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
    *input = Direction::LEFT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
    *input = Direction::RIGHT;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
    *input = Direction::UP;
    return true;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
    *input = Direction::DOWN;
    return true;
  }
  return false;
};

void SfmlAwsdInputController::setup() {};
#endif
