#pragma once

/**
 * A copy of the color enum from the library to avoid having an explicit
 * dependency on it in the generic user interface code.
 */
typedef enum Color : int {
        White = 0xFFFF,
        Black = 0x0000,
        Red = 0xF800,
        Green = 0x07E0,
        Blue = 0x001F,
        GreenBlue = 0X07FF,
        Magenta = 0xF81F,
        Cyan = 0x7FFF,
        Yellow = 0xFFE0,
        Brown = 0XBC40,
        BrownRed = 0XFC07,
        Gray = 0X8430,
        DarkBlue = 0X01CF,
        LightBlue = 0X7D7C,
        GrayBlue = 0X5458,
        LightGreen = 0XA651,
        LightGray = 0XC618,
        LightGrayBlue = 0X841F,
        MediumBlue = 0X2B12,
} Color;

const char *color_to_string(Color color);

Color get_good_contrast_text_color(Color color);
