// We use SFML only if running under the emulator
#ifdef EMULATOR
#include "sfml_hjkl_controller.hpp"
#include <SFML/Graphics.hpp>
#include <optional>

std::optional<Direction> SfmlHjklInputController::poll_for_direction()
{
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {
                return Direction::LEFT;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
                return Direction::RIGHT;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
                return Direction::UP;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
                return Direction::DOWN;
        }
        return std::nullopt;
};

void SfmlHjklInputController::setup() {};
#endif
