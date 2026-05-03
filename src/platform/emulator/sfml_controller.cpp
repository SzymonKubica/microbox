// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_controller.hpp"
#include <SFML/Graphics.hpp>
#include <optional>

std::optional<Direction> SfmlInputController::poll_for_direction()
{
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                return Direction::LEFT;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                return Direction::RIGHT;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                return Direction::UP;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                return Direction::DOWN;
        }
        return std::nullopt;
};

void SfmlInputController::setup() {};
#endif
