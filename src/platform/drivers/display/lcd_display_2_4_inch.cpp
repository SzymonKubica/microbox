#if defined(WAVESHARE_2_4_INCH_LCD)
#include "lcd_display_2_4_inch.hpp"
#include <Arduino.h>
#include <cstdint>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

#define DEV_BL_PIN 4

uint16_t to_tft_color(Color c) { return static_cast<uint16_t>(c); }

/**
 * Based on the supplied font size, we are converting it to the font number
 * used by TFT_eSPI library.
 */
int font_type_from_size(FontSize font_size)
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
void LcdDisplay::setup() const
{
        tft.init();
        tft.setRotation(3);
        analogWrite(DEV_BL_PIN, 140);
        tft.fillScreen(TFT_BLACK);
}
void LcdDisplay::initialize() const {};

void LcdDisplay::clear(Color color) const { tft.fillScreen(TFT_BLACK); };

void LcdDisplay::draw_rounded_border(Color color) const {};

void LcdDisplay::draw_circle(Point center, int radius, Color color,
                             int border_width, bool filled) const
{

        if (filled) {
                tft.fillCircle(center.x, center.y, radius,
                               to_tft_color(color));
        } else {

                tft.drawCircle(center.x, center.y, radius,
                               to_tft_color(color));
        }
};

void LcdDisplay::draw_rectangle(Point start, int width, int height, Color color,
                                int border_width, bool filled) const
{

        if (filled) {
                tft.fillRect(start.x , start.y , width ,
                             height , to_tft_color(color));
        } else {
                tft.drawRect(start.x , start.y , width ,
                             height , to_tft_color(color));
        }
};

void LcdDisplay::draw_rounded_rectangle(Point start, int width, int height,
                                        int radius, Color color) const
{
        tft.fillRoundRect(start.x , start.y , width ,
                          height , radius, to_tft_color(color));
};

void LcdDisplay::draw_line(Point start, Point end, Color color) const
{
        tft.drawLine(start.x, start.y, end.x, end.y, to_tft_color(color));
}

void LcdDisplay::draw_string(Point start, char *string_buffer,
                             FontSize font_size, Color bg_color,
                             Color fg_color) const
{
        tft.setTextColor(to_tft_color(fg_color), to_tft_color(bg_color));
        tft.setTextFont(font_type_from_size(font_size));
        tft.setTextSize(2);
        tft.setCursor(start.x, start.y);
        tft.print(string_buffer);
};

void LcdDisplay::clear_region(Point top_left, Point bottom_right,
                              Color clear_color) const

{
        tft.fillRect(top_left.x, top_left.y, bottom_right.x - top_left.x,
                     bottom_right.y - top_left.y, to_tft_color(clear_color));
};

// We return the width for the height as the display is mounted horizontally.
int LcdDisplay::get_height() const { return TFT_WIDTH; };

// We return the height for the width as the display is mounted horizontally.
int LcdDisplay::get_width() const { return TFT_HEIGHT; };

FontConfiguration LcdDisplay::get_font_configuration() const
{
        return FontConfiguration{
            .font_dimensions = {.width = 12, .height = 16},
            .heading_font_dimensions = {.width = 14, .height = 16}};
}
DisplayDimensions LcdDisplay::get_display_dimensions() const
{
        return DisplayDimensions{.width = TFT_HEIGHT,
                                 .height = TFT_WIDTH,
                                 .rounded_corner_radius =
                                     DISPLAY_CORNER_RADIUS};
}

int LcdDisplay::get_display_corner_radius() const
{
        return DISPLAY_CORNER_RADIUS;
}

bool LcdDisplay::refresh() const
{
        // This is a no-op as the display does not require refreshing
        return true;
}

void LcdDisplay::sleep() const
{
        // Turn display backlight off.
#define DEV_BL_PIN 4
        analogWrite(DEV_BL_PIN, 0);
        tft.writecommand(0x10); // ILI9341 SLEEP IN
}

void LcdDisplay::drawPixel(int32_t x, int32_t y, uint32_t color) {}
void LcdDisplay::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color,
                          uint32_t bg, uint8_t size)
{
}
void LcdDisplay::drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye,
                          uint32_t color)
{
        tft.drawLine(xs, ys, xe, ye, color);
}
void LcdDisplay::drawRect(int x, int y, int w, int h, int color)
{
        tft.drawRect(x, y, w, h, color);
}
void LcdDisplay::fillRect(int32_t x, int32_t y, int32_t w, int32_t h,
                          uint32_t color)
{
        tft.fillRect(x, y, w, h, color);
}
void LcdDisplay::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                               int32_t radius, uint32_t color)
{
        tft.drawRoundRect(x, y, w, h, radius, color);
}
void LcdDisplay::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                               int32_t radius, uint32_t color)
{
        tft.fillRoundRect(x, y, w, h, radius, color);
}
void LcdDisplay::drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color)
{
        tft.drawCircle(x, y, r, color);
}
void LcdDisplay::fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color)
{
        tft.fillCircle(x, y, r, color);
}
void LcdDisplay::drawString(const char *string, int32_t x, int32_t y)
{
        tft.drawString(string, x, y);
}

void LcdDisplay::drawEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                             uint32_t color)
{
        tft.drawEllipse(x, y, rx, ry, color);
}
void LcdDisplay::fillEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                             uint32_t color)
{
        tft.fillEllipse(x, y, rx, ry, color);
}
void LcdDisplay::fillScreen(uint32_t color) { tft.fillScreen(color); }
void LcdDisplay::setTextColor(uint32_t color) { tft.setTextColor(color); }
void LcdDisplay::setTextSize(uint8_t size) { tft.setTextSize(size); }

void LcdDisplay::drawTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                              int32_t xe, int32_t ye, uint32_t color)
{
        tft.drawTriangle(xs, ys, x2, y2, xe, ye, color);
}
void LcdDisplay::fillTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                              int32_t xe, int32_t ye, uint32_t color)
{
        tft.fillTriangle(xs, ys, x2, y2, xe, ye, color);
}

void LcdDisplay::pushImage(int x, int y, int width, int height,
                           const uint16_t *image_array)
{
        tft.pushImage(x, y, width, height, image_array);
}

TftCompatibleDisplay *LcdDisplay::cast_into_tft_compatible() { return this; }
#endif
