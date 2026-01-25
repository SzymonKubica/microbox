#include "lcd_display.hpp"
#include "../../../lib/GUI_Paint.h"
#include "../../../lib/LCD_Driver.h"

#define DISPLAY_CORNER_RADIUS 40
#define SCREEN_BORDER_WIDTH 3
void LcdDisplay::setup()
{
        Config_Init();
        LCD_Init();
        LCD_SetBacklight(50);
        Paint_Clear(BLACK);
};

void LcdDisplay::initialize()
{
        Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 270, WHITE);
};

void LcdDisplay::clear(Color color) { Paint_Clear(color); };

void LcdDisplay::draw_rounded_border(Color color)
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

void LcdDisplay::draw_circle(Point center, int radius, Color color,
                             int border_width, bool filled)
{
        int filled_repr = filled ? DRAW_FILL_FULL : DRAW_FILL_EMPTY;
        Paint_DrawCircle(center.x, center.y, radius, color,
                         static_cast<DOT_PIXEL>(border_width),
                         static_cast<DRAW_FILL>(filled_repr));
};

void LcdDisplay::draw_rectangle(Point start, int width, int height, Color color,
                                int border_width, bool filled)
{

        int filled_repr = filled ? DRAW_FILL_FULL : DRAW_FILL_EMPTY;

        /*
         * We need to add the adjustment because of the pixel-precision inconsistency
         * between the sfml emulator and the lcd display. The Paint_DrawRectangle
         * starts drawing one unit too far to the right and ends one unit too high,
         * so we need to add the adjustment
         */
        int adj = 1;
        Paint_DrawRectangle(start.x -1, start.y, start.x + width,
                            start.y + height +1, color,
                            static_cast<DOT_PIXEL>(border_width),
                            static_cast<DRAW_FILL>(filled_repr));
};

void LcdDisplay::draw_rounded_rectangle(Point start, int width, int height,
                                        int radius, Color color)
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

void LcdDisplay::draw_line(Point start, Point end, Color color)
{
        Paint_DrawLine(start.x, start.y, end.x, end.y, color, DOT_PIXEL_1X1,
                       LINE_STYLE_SOLID);
}

sFONT *map_font_size(FontSize font_size);
void LcdDisplay::draw_string(Point start, char *string_buffer,
                             FontSize font_size, Color bg_color, Color fg_color)
{
        Paint_DrawString_EN(start.x, start.y, string_buffer,
                            map_font_size(font_size), bg_color, fg_color);
};

void LcdDisplay::clear_region(Point top_left, Point bottom_right,
                              Color clear_color)
{
        Paint_ClearWindows(top_left.x, top_left.y, bottom_right.x,
                           bottom_right.y, clear_color);
};

sFONT *map_font_size(FontSize font_size)
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
int LcdDisplay::get_height() { return LCD_WIDTH; };

// We return the height for the width as the display is mounted horizontally.
int LcdDisplay::get_width() { return LCD_HEIGHT; };

int LcdDisplay::get_display_corner_radius() { return DISPLAY_CORNER_RADIUS; }

bool LcdDisplay::refresh()
{
        // This is a no-op as the display does not require refreshing
        return true;
}
