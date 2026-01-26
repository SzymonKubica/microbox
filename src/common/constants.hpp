/* Definitions of platform-specific constants that are commonly used by the
 * games.*/
#pragma once

#include "platform/interface/color.hpp"
#include <vector>
#define FONT_SIZE 16
#define HEADING_FONT_SIZE 24

#ifdef EMULATOR
#define HEADING_FONT_WIDTH 15
#endif
#ifndef EMULATOR
#define HEADING_FONT_WIDTH 17
#endif

// The font on the emulator is not pixel-accurate the same as what we
// have on the actual hardware. Because of this we need this conditional
// constant definition.
#ifdef EMULATOR
#define FONT_WIDTH 10
#endif
#ifndef EMULATOR
#define FONT_WIDTH 11
#endif

#define SCREEN_BORDER_WIDTH 3

/* Constants below control time intervals between input polling */
#define INPUT_POLLING_DELAY 50

#ifdef EMULATOR
// On the emulator we are using the keyboard which is faster than the keypad
// buttons on the Arduino input shield, because of this we can afford a shorter
// timeout to make the experience more snappy.
#define MOVE_REGISTERED_DELAY 100
#endif
#ifndef EMULATOR
#define MOVE_REGISTERED_DELAY 150
#endif

constexpr int DISPLAY_HEIGHT = 240;
constexpr int DISPLAY_WIDTH = 280;
constexpr int DISPLAY_CORNER_RADIUS = 40;

extern const std::vector<Color> AVAILABLE_COLORS;
