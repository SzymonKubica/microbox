#include "../common/platform/interface/platform.hpp"
#include "../common/user_interface.hpp"
#include <optional>

typedef enum Game
    : int { Unknown = 0,
            MainMenu = 1,
            Clean2048 = 2,
            Minesweeper = 3,
            GameOfLife = 4,
            Settings = 5,
            RandomSeedPicker = 6,
    } Game;

typedef struct GameMenuConfiguration {
        Game game;
        Color accent_color;
        UserInterfaceRenderingMode rendering_mode;
        bool show_help_text;
} GameMenuConfiguration;

extern Game game_from_string(const char *name);

extern const char *game_to_string(Game game);

void select_game(Platform *p);

/**
 * Similar to `collect_configuration` from `configuration.hpp`, it returns true
 * if the configuration was successfully collected. Otherwise, if the user
 * requested exit by pressing the blue button, it returns false and this needs
 * to be handled by the main game loop.
 */
std::optional<UserAction> collect_game_menu_config(Platform *p,
                              GameMenuConfiguration *configuration);
