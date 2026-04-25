#if defined(WAVESHARE_2_4_INCH_LCD)
#include "lcd_display_2_4_inch.hpp"
#include <Arduino.h>
#include <cstdint>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

#define DEV_BL_PIN 4

uint16_t to_tft_color(Color c) { return static_cast<uint16_t>(c); }

int map_font_size(FontSize font_size)
{
        switch (font_size) {
        case Size16:
                return 1;
        case Size24:
                return 2;
        default:
                return 1;
        }
}

// TODO: ensure that this can be set to 0 without bricking the console.
#define DISPLAY_CORNER_RADIUS 40
#define SCREEN_BORDER_WIDTH 3
void LcdDisplay::setup()
{
        tft.init();
        tft.setRotation(3);
        analogWrite(DEV_BL_PIN, 140);
        tft.fillScreen(TFT_BLACK);
}
void LcdDisplay::initialize() {};

void LcdDisplay::clear(Color color) { tft.fillScreen(TFT_BLACK); };

void LcdDisplay::draw_rounded_border(Color color) {};

void LcdDisplay::draw_circle(Point center, int radius, Color color,
                             int border_width, bool filled)
{

        int adj = 1;
        if (filled) {
                tft.fillCircle(center.x - adj, center.y - adj, radius,
                               to_tft_color(color));
        } else {

                tft.drawCircle(center.x - adj, center.y - adj, radius,
                               to_tft_color(color));
        }
};

void LcdDisplay::draw_rectangle(Point start, int width, int height, Color color,
                                int border_width, bool filled)
{

        int adj = 1;
        if (filled) {
                tft.fillRect(start.x - adj, start.y - adj, width + adj,
                             height + adj, to_tft_color(color));
        } else {
                tft.drawRect(start.x - adj, start.y - adj, width + adj,
                             height + adj, to_tft_color(color));
        }
};

void LcdDisplay::draw_rounded_rectangle(Point start, int width, int height,
                                        int radius, Color color)
{
        draw_rectangle(start, width, height, color, 1, false);
};

void LcdDisplay::draw_line(Point start, Point end, Color color)
{
        tft.drawLine(start.x, start.y - 2, end.x, end.y - 2,
                     to_tft_color(color));
}

void LcdDisplay::draw_string(Point start, char *string_buffer,
                             FontSize font_size, Color bg_color, Color fg_color)
{
        tft.setTextColor(to_tft_color(fg_color), to_tft_color(bg_color));
        tft.setTextFont(map_font_size(font_size));
        tft.setTextSize(2);
        tft.setCursor(start.x, start.y);
        tft.print(string_buffer);
};

void LcdDisplay::clear_region(Point top_left, Point bottom_right,
                              Color clear_color)

{
        int adj = 0;
        tft.fillRect(top_left.x, top_left.y - adj, bottom_right.x - top_left.x,
                     bottom_right.y - top_left.y - adj,
                     to_tft_color(clear_color));
};

// We return the width for the height as the display is mounted horizontally.
int LcdDisplay::get_height() { return TFT_WIDTH; };

// We return the height for the width as the display is mounted horizontally.
int LcdDisplay::get_width() { return TFT_HEIGHT; };

int LcdDisplay::get_display_corner_radius() { return DISPLAY_CORNER_RADIUS; }

bool LcdDisplay::refresh()
{
        // This is a no-op as the display does not require refreshing
        return true;
}

void LcdDisplay::sleep()
{
        tft.writecommand(0x10); // ILI9341 SLEEP IN
}
#endif
