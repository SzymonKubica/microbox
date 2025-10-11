#pragma once
// TODO: think about those dependencies. Should they be a part of the platform
// definition?
#include "../../point.hpp"
#include "../../font_size.hpp"
#include "color.hpp"

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
        virtual void setup() = 0;
        /**
         * Initializes the display, this is for actions such as erasing the
         * previously rendered shapes in a physical Arduino display.
         */
        virtual void initialize() = 0;
        /**
         * Clears the display. This is done by redrawing the entire screen with
         * the specified color.
         */
        virtual void clear(Color color) = 0;
        /**
         * Draws a rounded border around the screen. This is needed due to the
         * specifics of the physical display used by the game console: the LCD
         * screen has rounded corners and we need a utility function that draws
         * a border perfectly encircling the screen. The emulated
         * implementations of the display do not necessarily need to provide
         * this functionality.
         */
        virtual void draw_rounded_border(Color color) = 0;
        /**
         * Draws a circle with specified color, border width and fill.
         */
        virtual void draw_circle(Point center, int radius, Color color,
                                 int border_width, bool filled) = 0;
        /**
         * Draws a rectangle with specified color, border width and fill.
         */
        virtual void draw_rectangle(Point start, int width, int height,
                                    Color color, int border_width,
                                    bool filled) = 0;
        /**
         * Draws a rounded rectangle with specified color. This is useful for
         * drawing nicely-looking game menu items.
         */
        virtual void draw_rounded_rectangle(Point start, int width, int height,
                                            int radius, Color color) = 0;
        /**
         * Draws a line from a start point to the end point with specified
         * color. Note that fill and thickness are not controllable yet.
         */
        virtual void draw_line(Point start, Point end, Color color) = 0;
        /**
         * Prints a string on the display, allows for specifying the font size,
         * color and background color.
         */
        virtual void draw_string(Point start, char *string_buffer,
                                 FontSize font_size, Color bg_color,
                                 Color fg_color) = 0;
        /**
         * Clears a rectangular region of the display. This is done by redrawing
         * the rectangle using the specified color. Note that on the physical
         * display this operation is potentially slow, hence we need to redraw
         * small regions at a time if we want the game to remain usable.
         */
        virtual void clear_region(Point top_left, Point bottom_right,
                                  Color clear_color) = 0;

        /**
         * Returns the height of the display.
         */
        virtual int get_height() = 0;

        /**
         * Returns the width of the display.
         */
        virtual int get_width() = 0;

        /**
         * For displays with rounded corners it returns the radius in pixels.
         * This is needed for drawing borders with rounded corners around the
         * display.
         */
        virtual int get_display_corner_radius() = 0;

        /**
         * For displays that require redrawing every frame, we need to provide
         * ability to refresh their contents. Note that on the arduino LCD
         * display this will be a no-op as that display does not require
         * refreshing.
         */
        virtual void refresh() = 0;
};
