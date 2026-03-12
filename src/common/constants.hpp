/* Definitions of platform-specific constants that are commonly used by the
 * games.*/
#pragma once

#include "platform/interface/color.hpp"
#include <vector>

#if defined(WAVESHARE_2_4_INCH_LCD)
#define FONT_SIZE 16
#define HEADING_FONT_SIZE 16
#else
#define FONT_SIZE 16
#define HEADING_FONT_SIZE 24
#endif


#ifdef EMULATOR
#define HEADING_FONT_WIDTH 15
#else
#if defined(WAVESHARE_2_4_INCH_LCD)
#define HEADING_FONT_WIDTH 12
#else
#define HEADING_FONT_WIDTH 17
#endif
#endif

// The font on the emulator is not pixel-accurate the same as what we
// have on the actual hardware. Because of this we need this conditional
// constant definition.
#ifdef EMULATOR
#define FONT_WIDTH 10
#endif
#ifndef EMULATOR
#if defined(WAVESHARE_2_4_INCH_LCD)
#define FONT_WIDTH 12
#else
#define FONT_WIDTH 11
#endif
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
#if defined(WAVESHARE_2_4_INCH_LCD)
constexpr int DISPLAY_WIDTH = 320;
constexpr int DISPLAY_CORNER_RADIUS = 40;
#else
constexpr int DISPLAY_WIDTH = 280;
constexpr int DISPLAY_CORNER_RADIUS = 40;
#endif

extern const std::vector<Color> AVAILABLE_COLORS;
