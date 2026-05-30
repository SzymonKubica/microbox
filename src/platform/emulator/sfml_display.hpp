#ifdef EMULATOR
#pragma once
#include "../interface/display.hpp"
#include <SFML/Graphics.hpp>

/**
 * @brief SfmlDisplay class that implements the Display interface for the
 * physical SFML library. This is used for emulating the console behaviour on
 * linux machines.
 */
class SfmlDisplay : public Display, public TftCompatibleDisplay
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
        void setup() const override;
        /**
         * Initializes the display, this is for actions such as erasing the
         * previously rendered shapes in a physical Arduino display. Intended
         * for use in the body of the `loop` Aruino function.
         */
        void initialize() const override;
        /**
         * Clears the display. This is done by redrawing the entire screen with
         * the specified color.
         */
        void clear(Color color) const override;
        /**
         * Draws a rounded border around the screen. This is needed due to the
         * specifics of the physical display used by the game console: the LCD
         * screen has rounded corners and we need a utility function that draws
         * a border perfectly encircling the screen. The emulated
         * implementations of the display do not necessarily need to provide
         * this functionality.
         */
        void draw_rounded_border(Color color) const override;
        /**
         * Draws a circle with specified color, border width and fill.
         */
        void draw_circle(Point center, int radius, Color color,
                         int border_width, bool filled) const override;
        /**
         * Draws a rectangle with specified color, border width and fill.
         */
        void draw_rectangle(Point start, int width, int height, Color color,
                            int border_width, bool filled) const override;
        /**
         * Draws a rounded rectangle with specified color. This is useful for
         * drawing nicely-looking game menu items.
         */
        void draw_rounded_rectangle(Point start, int width, int height,
                                    int radius, Color color) const override;
        /**
         * Draws a line from a start point to the end point with specified
         * color. Note that fill and thickness are not controllable yet.
         */
        void draw_line(Point start, Point end, Color color) const override;
        /**
         * Prints a string on the display, allows for specifying the font size,
         * color and background color.
         */
        void draw_string(Point start, char *string_buffer, FontSize font_size,
                         Color bg_color, Color fg_color) const override;
        /**
         * Clears a rectangular region of the display. This is done by redrawing
         * the rectangle using the specified color. Note that on the physical
         * display this operation is potentially slow, hence we need to redraw
         * small regions at a time if we want the game to remain usable.
         */
        void clear_region(Point top_left, Point bottom_right,
                          Color clear_color) const override;

        /**
         * Returns the height of the display.
         */
        int get_height() const override;

        /**
         * Returns the width of the display.
         */
        int get_width() const override;

        /**
         * For displays with rounded corners it returns the radius in pixels.
         * This is needed for drawing borders with rounded corners around the
         * display.
         */
        int get_display_corner_radius() const override;

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
        bool refresh() const override;

        /**
         * For physical displays this is supposed to send a command to the
         * display driver to turn off and turn the display backlight to save
         * battery.
         */
        void sleep() const override;

        void draw_image(Point start, int width, int height,
                        const uint16_t *bitmap) const override;

        SfmlDisplay(sf::RenderWindow *window, sf::RenderTexture *texture)
            : window(window), texture(texture)
        {
        }

        void drawPixel(int32_t x, int32_t y, uint32_t color) override;
        void drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color,
                      uint32_t bg, uint8_t size) override;
        void drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye,
                      uint32_t color) override;
        void drawRect(int x, int y, int w, int h, int color) override;
        void fillRect(int32_t x, int32_t y, int32_t w, int32_t h,
                      uint32_t color) override;
        void drawTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                          int32_t xe, int32_t ye, uint32_t color) override;
        void fillTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                          int32_t xe, int32_t ye, uint32_t color) override;
        void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                           int32_t radius, uint32_t color) override;
        void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                           int32_t radius, uint32_t color) override;
        void drawCircle(int32_t x, int32_t y, int32_t r,
                        uint32_t color) override;
        void fillCircle(int32_t x, int32_t y, int32_t r,
                        uint32_t color) override;
        void drawString(const char *string, int32_t x, int32_t y) override;
        void fillScreen(uint32_t color) override;
        void setTextColor(uint32_t color) override;
        void setTextSize(uint8_t size) override;

        TftCompatibleDisplay *cast_into_tft_compatible() override;

      private:
        sf::RenderWindow *window;
        sf::RenderTexture *texture;

        /**
         * Font size that was set by the `setTextSize` method on the
         * TftCompatibleDisplay We need to maintain this state to achieve
         * compatibility with the interface exposed by the TFT_eSPI library.
         */
        uint32_t tft_font_size;
        /**
         * Text color that was set by the `setTextColor` method on the
         * TftCompatibleDisplay We need to maintain this state to achieve
         * compatibility with the interface exposed by the TFT_eSPI library.
         */
        sf::Color tft_text_color;
};
#endif
