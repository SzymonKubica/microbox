#pragma once
#include "../../common/point.hpp"
#include "../../common/font_size.hpp"
#include "../../common/color.hpp"
#include <cstdint>

/*
 * @brief Display interface that needs to be implemented by classes that will be
 * used for drawing our games.
 *
 */
class Display
{
      public:
        /**
         * Performs the setup of the display. This is intended for performing
         * initialization of the modules that are responsible for driving the
         * particular implementation of the display. In case of the hardware
         * display, this is supposed to initialize the display driver and erease
         * its previous contents. Note that this should be called inside of
         * the `setup` Arduino function and is intended to be executed only
         * once.
         */
        virtual void setup() const = 0;
        /**
         * Initializes the display, this is for actions such as erasing the
         * previously rendered shapes in a physical Arduino display.
         */
        virtual void initialize() const = 0;
        /**
         * Clears the display. This is done by redrawing the entire screen with
         * the specified color.
         */
        virtual void clear(Color color) const = 0;
        /**
         * Draws a rounded border around the screen. This is needed due to the
         * specifics of the physical display used by the game console: the LCD
         * screen has rounded corners and we need a utility function that draws
         * a border perfectly encircling the screen. The emulated
         * implementations of the display do not necessarily need to provide
         * this functionality.
         */
        virtual void draw_rounded_border(Color color) const = 0;
        /**
         * Draws a circle with specified color, border width and fill.
         */
        virtual void draw_circle(Point center, int radius, Color color,
                                 int border_width, bool filled) const = 0;
        /**
         * Draws a rectangle with specified color, border width and fill.
         */
        virtual void draw_rectangle(Point start, int width, int height,
                                    Color color, int border_width,
                                    bool filled) const = 0;
        /**
         * Draws a rounded rectangle with specified color. This is useful for
         * drawing nicely-looking game menu items.
         */
        virtual void draw_rounded_rectangle(Point start, int width, int height,
                                            int radius, Color color) const = 0;
        /**
         * Draws a line from a start point to the end point with specified
         * color. Note that fill and thickness are not controllable yet.
         */
        virtual void draw_line(Point start, Point end, Color color) const = 0;
        /**
         * Prints a string on the display, allows for specifying the font size,
         * color and background color.
         */
        virtual void draw_string(Point start, char *string_buffer,
                                 FontSize font_size, Color bg_color,
                                 Color fg_color) const = 0;
        /**
         * Clears a rectangular region of the display. This is done by redrawing
         * the rectangle using the specified color. Note that on the physical
         * display this operation is potentially slow, hence we need to redraw
         * small regions at a time if we want the game to remain usable.
         */
        virtual void clear_region(Point top_left, Point bottom_right,
                                  Color clear_color) const = 0;

        /**
         * Returns the height of the display.
         */
        virtual int get_height() const = 0;

        /**
         * Returns the width of the display.
         */
        virtual int get_width() const = 0;

        /**
         * For displays with rounded corners it returns the radius in pixels.
         * This is needed for drawing borders with rounded corners around the
         * display.
         */
        virtual int get_display_corner_radius() const = 0;

        /**
         * For displays that require redrawing every frame, we need to provide
         * ability to refresh their contents. Note that on the arduino LCD
         * display this will be a no-op as that display does not require
         * refreshing. On the emulator, this is required so that SFML can
         * inspect the state of the window. If the window is closed, the refresh
         * 'has failed' and this function returns false. Note that on the
         * physical LCD display false willl never be returned.
         *
         * The reason this window close exception handling is implemented with
         * a boolean flag instead of proper exceptions is that Arduino does
         * not support exceptions, hence we would need to gate the
         * emulator-specific exception handling code behind conditional
         * compilation which would make the code ugly and cluttered.
         */
        virtual bool refresh() const = 0;

        /**
         * For physical displays this is supposed to send a command to the
         * display driver to turn off and turn the display backlight to save
         * battery.
         */
        virtual void sleep() const = 0;

        // We make this an impure virtual function to avoid implementing it in
        // all types of displays at this point. TODO: clean it up and provide
        // an implementation everywhere.
        virtual void draw_image(Point start, int width, int height,
                                const uint16_t *bitmap) const
        {
        }
};

/**
 * A display interface that is drop-in compatible with the interface exposed
 * by the TFT_eSPI library for LCD displays. The idea here is to expose an
 * interface that is exactly the same so that we can generate images using
 * Lopaka (see here: https://lopaka.app/) and paste them into the source code.
 *
 * Note that not all functions on the TFT_eSPI interface are here.
 * We are only interested in functions that handle graphics drawing. The aim
 * here is to have the main display abstraction above handle initialization
 * and other housekeeping items.
 */
class TftCompatibleDisplay
{
      public:
        virtual void drawPixel(int32_t x, int32_t y, uint32_t color) = 0;
        virtual void drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color,
                              uint32_t bg, uint8_t size) = 0;
        virtual void drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye,
                              uint32_t color) = 0;
        virtual void drawRect(int x, int y, int w, int h,
                              int color) = 0;
        virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h,
                              uint32_t color) = 0;
        virtual void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                   int32_t radius, uint32_t color) = 0;
        virtual void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                   int32_t radius, uint32_t color) = 0;
        virtual void drawCircle(int32_t x, int32_t y, int32_t r,
                                uint32_t color) = 0;
        virtual void fillCircle(int32_t x, int32_t y, int32_t r,
                                uint32_t color) = 0;
        virtual void drawString(const char *string, int32_t x, int32_t y) = 0;
        virtual void fillScreen(uint32_t color) = 0;
        virtual void setTextColor(uint32_t color) = 0;
        // Set character size multiplier (this increases pixel size)
        virtual void setTextSize(uint8_t size) = 0;
};
