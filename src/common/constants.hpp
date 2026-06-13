/* Definitions of platform-specific constants that are commonly used by the
 * games.*/
#pragma once

#include "../common/color.hpp"
#include <vector>

/* Constants below control time intervals between input polling */
#define INPUT_POLLING_DELAY 50
#define MOVE_REGISTERED_DELAY 150

#define SCREEN_BORDER_WIDTH 3

extern const std::vector<Color> AVAILABLE_COLORS;
