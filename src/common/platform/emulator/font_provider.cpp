#ifdef EMULATOR
#include "font_provider.hpp"
#include "emulator_config.h"

const sf::Font get_emulator_font()
{
        std::string source_dir(CMAKE_SOURCE_DIR);
        const sf::Font font(
            source_dir + "/assets/emulator/JetBrainsMonoNerdFont-Regular.ttf");
        return font;
}
#endif
