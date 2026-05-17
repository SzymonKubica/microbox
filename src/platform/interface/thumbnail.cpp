#include "thumbnail.hpp"
#include "string.h"
#include "../../common/user_interface.hpp"

void NameBoxRenderer::render_thumbnail(
    const Platform &platform, const UserInterfaceCustomization &customization)
{

        const auto &display = *platform.display;
        display.clear(Black);
        const char *header = "App";
        render_config_bar_centered(display, 50, strlen(header),
                                   strlen(option_name), header, option_name,
                                   false, true, true, customization);
}
