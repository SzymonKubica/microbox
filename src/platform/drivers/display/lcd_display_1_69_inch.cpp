#if defined(WAVESHARE_1_69_INCH_LCD)
#include "lcd_display_1_69_inch.hpp"
#include "../../../lib/waveshare_1_69_inch_lcd/GUI_Paint.h"
#include "../../../lib/waveshare_1_69_inch_lcd/LCD_Driver.h"

#define DISPLAY_CORNER_RADIUS 40
#define SCREEN_BORDER_WIDTH 3
void LcdDisplay_1_69::setup() const
{
        Config_Init();
        LCD_Init();
        LCD_SetBacklight(50);
        Paint_Clear(BLACK);
};

void LcdDisplay_1_69::initialize() const
{
        Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 270, WHITE);
};

void LcdDisplay_1_69::clear(Color color) const { Paint_Clear(color); };

void LcdDisplay_1_69::draw_rounded_border(Color color) const
{
        int rounding_radius = DISPLAY_CORNER_RADIUS;
        int margin = SCREEN_BORDER_WIDTH;
        int line_width = 2;
        Point top_left_corner = {.x = rounding_radius + margin,
                                 .y = rounding_radius + margin};
        Point bottom_right_corner = {.x = LCD_HEIGHT - rounding_radius - margin,
                                     .y = LCD_WIDTH - rounding_radius - margin};

        int x_positions[2] = {top_left_corner.x, bottom_right_corner.x};
        int y_positions[2] = {top_left_corner.y, bottom_right_corner.y};

        Paint_Clear(BLACK);

        // Draw the four rounded corners.
        for (int x : x_positions) {
                for (int y : y_positions) {
                        Paint_DrawCircle(x, y, rounding_radius, color,
                                         DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
                }
        }

        // Draw the four lines connecting the circles.
        Paint_DrawLine(margin, top_left_corner.y, margin, bottom_right_corner.y,
                       color, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

        Paint_DrawLine(LCD_HEIGHT - margin, top_left_corner.y,
                       LCD_HEIGHT - margin, bottom_right_corner.y, color,
                       DOT_PIXEL_3X3, LINE_STYLE_SOLID);

        Paint_DrawLine(top_left_corner.x, margin, bottom_right_corner.x, margin,
                       color, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

        Paint_DrawLine(top_left_corner.x, LCD_WIDTH - margin,
                       bottom_right_corner.x, LCD_WIDTH - margin, color,
                       DOT_PIXEL_3X3, LINE_STYLE_SOLID);

        // Erase the middle bits of the four circles
        Paint_ClearWindows(margin + line_width, top_left_corner.y,
                           margin + line_width + 2 * rounding_radius,
                           top_left_corner.y + rounding_radius + line_width,
                           BLACK);

        Paint_ClearWindows(
            LCD_HEIGHT - margin - line_width - 1 - 2 * rounding_radius,
            top_left_corner.y, LCD_HEIGHT - margin - line_width - 1,
            top_left_corner.y + rounding_radius + line_width, BLACK);

        Paint_ClearWindows(margin + line_width,
                           bottom_right_corner.y - rounding_radius -
                               line_width - margin,
                           margin + line_width + 2 * rounding_radius,
                           bottom_right_corner.y, BLACK);

        Paint_ClearWindows(
            LCD_HEIGHT - margin - line_width - 1 - 2 * rounding_radius,
            bottom_right_corner.y - rounding_radius - line_width - margin,
            LCD_HEIGHT - margin - line_width - 1, bottom_right_corner.y, BLACK);

        // The four remaining vertical lines
        Paint_ClearWindows(top_left_corner.x, margin + line_width,
                           top_left_corner.x + rounding_radius + line_width,
                           margin + line_width + rounding_radius, BLACK);

        Paint_ClearWindows(top_left_corner.x,
                           LCD_WIDTH - margin - line_width - 1 -
                               rounding_radius,
                           top_left_corner.x + rounding_radius + line_width,
                           LCD_WIDTH - margin - line_width - 1, BLACK);

        Paint_ClearWindows(
            bottom_right_corner.x - rounding_radius - line_width - 1,
            margin + line_width, bottom_right_corner.x - line_width - 1,
            margin + line_width + rounding_radius, BLACK);

        Paint_ClearWindows(
            bottom_right_corner.x - rounding_radius - line_width - 1,
            LCD_WIDTH - margin - line_width - 1 - rounding_radius,
            bottom_right_corner.x - line_width - 1,
            LCD_WIDTH - margin - line_width - 1, BLACK);
};

void LcdDisplay_1_69::draw_circle(Point center, int radius, Color color,
                                  int border_width, bool filled) const
{
        int filled_repr = filled ? DRAW_FILL_FULL : DRAW_FILL_EMPTY;

        Paint_DrawCircle(center.x, center.y, radius, color,
                         static_cast<DOT_PIXEL>(border_width),
                         static_cast<DRAW_FILL>(filled_repr));
};

void LcdDisplay_1_69::draw_rectangle(Point start, int width, int height,
                                     Color color, int border_width,
                                     bool filled) const
{
        int filled_repr = filled ? DRAW_FILL_FULL : DRAW_FILL_EMPTY;

        /**
         * Because of stupid implementation details, drawing a rectangle that
         * is filled excludes the last line (while it is included if you draw
         * the rectangle as 4 lines with no fill).
         * Because of this, we need to override the height of the rectangle
         * depending on whether it is filled or not.
         */
        int height_adjustment = filled ? 0 : 1;

        Paint_DrawRectangle(start.x, start.y, start.x + width - 1,
                            start.y + height - height_adjustment, color,
                            static_cast<DOT_PIXEL>(border_width),
                            static_cast<DRAW_FILL>(filled_repr));
};

void LcdDisplay_1_69::draw_rounded_rectangle(Point start, int width, int height,
                                             int radius, Color color) const
{

        Point top_left_corner = {.x = start.x + radius, .y = start.y + radius};

        Point bottom_right_corner = {.x = start.x + width - radius,
                                     .y = start.y + height - radius};

        int x_positions[2] = {top_left_corner.x, bottom_right_corner.x};
        int y_positions[2] = {top_left_corner.y, bottom_right_corner.y};

        // Draw the four rounded corners.
        for (int x : x_positions) {
                for (int y : y_positions) {
                        Paint_DrawCircle(x, y, radius, color, DOT_PIXEL_1X1,
                                         DRAW_FILL_FULL);
                }
        }

        Paint_DrawRectangle(top_left_corner.x, start.y,
                            start.x + width - radius, start.y + radius, color,
                            DOT_PIXEL_1X1, DRAW_FILL_FULL);

        Paint_DrawRectangle(start.x, top_left_corner.y, start.x + width + 1,
                            bottom_right_corner.y, color, DOT_PIXEL_1X1,
                            DRAW_FILL_FULL);

        // +1 is because the endY bound is not included
        Paint_DrawRectangle(top_left_corner.x, start.y + height - radius,
                            start.x + width - radius, start.y + height + 1,
                            color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
};

void LcdDisplay_1_69::draw_line(Point start, Point end, Color color) const
{
        Paint_DrawLine(start.x, start.y, end.x, end.y, color,
                       DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

sFONT *map_font_size_1_69_specific(FontSize font_size);
void LcdDisplay_1_69::draw_string(Point start, char *string_buffer,
                                  FontSize font_size, Color bg_color,
                                  Color fg_color) const
{
        Paint_DrawString_EN(start.x, start.y, string_buffer,
                            map_font_size_1_69_specific(font_size), bg_color,
                            fg_color);
};

void LcdDisplay_1_69::clear_region(Point top_left, Point bottom_right,
                                   Color clear_color) const

{

        // Clearing is also inclusive and goes too far compared to the other
        // displays. We need to adjust here to bring the behaviour in line with
        // the TFT_eSPI display that we are using for the other target
        // microcontrollers. For some reason it also starts about 1 pixel off
        // to the right so we need to decrease the starting point.
        int adj = 1;
        Paint_ClearWindows(top_left.x - adj, top_left.y, bottom_right.x - adj,
                           bottom_right.y - adj, clear_color);
};

sFONT *map_font_size_1_69_specific(FontSize font_size)
{
        switch (font_size) {
        case Size16:
                return &Font16;
        case Size24:
                return &Font24;
        default:
                return &Font16;
        }
}

// We return the width for the height as the display is mounted horizontally.
int LcdDisplay_1_69::get_height() const { return LCD_WIDTH; };

// We return the height for the width as the display is mounted horizontally.
int LcdDisplay_1_69::get_width() const { return LCD_HEIGHT; };

FontConfiguration LcdDisplay_1_69::get_font_configuration() const
{
        return FontConfiguration{
            .font_dimensions = {.width = 11, .height = 16},
            .heading_font_dimensions = {.width = 17, .height = 24}};
}
DisplayDimensions LcdDisplay_1_69::get_display_dimensions() const
{
        return DisplayDimensions{.width = LCD_HEIGHT,
                                 .height = LCD_WIDTH,
                                 .rounded_corner_radius =
                                     DISPLAY_CORNER_RADIUS};
}

int LcdDisplay_1_69::get_display_corner_radius() const
{
        return DISPLAY_CORNER_RADIUS;
}
bool LcdDisplay_1_69::refresh() const
{
        // This is a no-op as the display does not require refreshing
        return true;
}

void LcdDisplay_1_69::sleep() const
{
        // This is a no-op as we don't support display sleeping on
        // the Arduino waveshare 1.69 inch LCD. This is because those
        // console targets are designed with a power-bank in mind (no LiPo
        // batteries) and so we don't need functionality to sleep the display.
}

/**
 * Note that the Waveshare 1.69 inch LCD does not use the same display driver
 * as the 2.4 inch LCD, hence it is not copatible with the TFT_eSPI libarry and
 * cannot be used with the TFT-compatible interface. This means that all
 * lopaka-generated UI artifact code is not compatible with this display.
 *
 * This is mostly fine as this display doesn't refresh fast enough for those
 * UI elements to look good. Adding them would result in the UI feeling laggy.
 */
TftCompatibleDisplay *LcdDisplay_1_69::cast_into_tft_compatible()
{
        return nullptr;
}
#endif
