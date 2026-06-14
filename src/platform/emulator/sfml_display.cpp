#include "font_provider.hpp"
#ifdef EMULATOR
#include <SFML/Graphics/Color.hpp>
#include "sfml_display.hpp"
#include <SFML/Graphics.hpp>
#include "../../common/logging.hpp"

#define SCREEN_BORDER_WIDTH 3
#define TAG "sfml_display"

void SfmlDisplay::setup() const {};

void SfmlDisplay::initialize() const {}

void SfmlDisplay::sleep() const {};

sf::Color map_to_sf_color(uint32_t color);
void SfmlDisplay::clear(Color color) const
{
        texture->clear(map_to_sf_color(color));
        texture->display();
};

/**
 * Draws a rounded border around the game display, this might not be the most
 * efficient solution using the SFML API, but it uses the same logic as the
 * LCD display implementation for consistency.
 */
void SfmlDisplay::draw_rounded_border(Color color) const
{
        int rounding_radius = DISPLAY_CORNER_RADIUS;
        int margin = SCREEN_BORDER_WIDTH;
        int line_width = 3;
        int width = get_width();
        int height = get_height();
        Point top_left_corner = {.x = rounding_radius + margin,
                                 .y = rounding_radius + margin};
        Point bottom_right_corner = {.x = width - rounding_radius - margin,
                                     .y = height - rounding_radius - margin};

        int x_positions[2] = {top_left_corner.x, bottom_right_corner.x};
        int y_positions[2] = {top_left_corner.y, bottom_right_corner.y};

        clear(Black);

        // Draw the four rounded corners.
        for (int x : x_positions) {
                for (int y : y_positions) {
                        draw_circle({.x = x, .y = y}, rounding_radius, color, 3,
                                    false);
                }
        }

        // Draw the four lines connecting the circles.
        // This is the first vertical line on the left
        draw_rectangle({.x = 0, .y = top_left_corner.y}, 0,
                       bottom_right_corner.y - top_left_corner.y, color, 3,
                       true);

        // This is the second vertical line on the left
        draw_rectangle({.x = width, .y = top_left_corner.y}, 0,
                       bottom_right_corner.y - top_left_corner.y, color, 3,
                       true);

        // This is the first horizontal line at the top
        draw_rectangle({.x = top_left_corner.x, .y = 0},
                       bottom_right_corner.x - top_left_corner.x, 0, color, 3,
                       true);

        // This is the second horizontal line at the bottom
        draw_rectangle({.x = top_left_corner.x, .y = height},
                       bottom_right_corner.x - top_left_corner.x, 0, color, 3,
                       true);

        int circle_diameter = 2 * rounding_radius;
        // Erase the middle bits of the four circles
        // This clears bottom half of the top left circle
        clear_region({.x = margin, .y = top_left_corner.y - margin},
                     {.x = margin + line_width + circle_diameter,
                      .y = top_left_corner.y + rounding_radius + line_width},
                     Black);

        // This clears bottom half of the top right circle
        clear_region({.x = width - margin - line_width - circle_diameter - 1,
                      .y = top_left_corner.y - margin},
                     {.x = width - margin,
                      .y = top_left_corner.y + rounding_radius + line_width},
                     Black);

        // This clears top half of the bottom left circle
        clear_region({.x = margin,
                      .y = bottom_right_corner.y - rounding_radius -
                           line_width - margin},
                     {.x = margin + line_width + circle_diameter,
                      .y = bottom_right_corner.y + margin},
                     Black);

        // This clears top half of the bottom right circle
        clear_region({.x = width - margin - line_width - circle_diameter - 1,
                      .y = bottom_right_corner.y - rounding_radius - margin},
                     {.x = width - margin, .y = bottom_right_corner.y + margin},
                     Black);

        // The four remaining lines
        clear_region({.x = top_left_corner.x - margin, .y = margin},
                     {.x = top_left_corner.x + rounding_radius + line_width,
                      .y = margin + line_width + rounding_radius},
                     Black);

        clear_region(
            {.x = top_left_corner.x - margin,
             .y = height - margin - line_width - 1 - rounding_radius},
            {.x = top_left_corner.x + rounding_radius + line_width + margin,
             .y = height - margin},
            Black);

        clear_region(
            {.x = bottom_right_corner.x - rounding_radius - line_width - 1,
             .y = margin},
            {.x = bottom_right_corner.x + margin,
             .y = margin + line_width + rounding_radius},
            Black);

        clear_region(
            {.x = bottom_right_corner.x - rounding_radius - line_width - 1,
             .y = height - margin - line_width - 1 - rounding_radius},
            {.x = bottom_right_corner.x - line_width - 1, .y = height - margin},
            Black);
}

void SfmlDisplay::draw_circle(Point center, int radius, Color color,
                              int border_width, bool filled) const
{
        // Note: the circle is always filled, given the current use cases this
        // is fine, but we need to tighten up the API in the future as we
        // start onboarding more complex game rendering.
        sf::CircleShape circle(radius);
        circle.setPosition(
            {(float)(center.x - radius), (float)(center.y - radius)});

        if (filled) {
                circle.setFillColor(map_to_sf_color(color));
        } else {
                circle.setFillColor(map_to_sf_color(Black));
        }

        circle.setOutlineColor(map_to_sf_color(color));
        circle.setOutlineThickness(-border_width);
        texture->draw(circle);
        texture->display();
};

void SfmlDisplay::draw_rectangle(Point start, int width, int height,
                                 Color color, int border_width,
                                 bool filled) const
{
        sf::RectangleShape rectangle({(float)width, (float)height});
        rectangle.setPosition({(float)start.x, (float)start.y});
        if (filled) {
                rectangle.setFillColor(map_to_sf_color(color));
        } else {
                rectangle.setFillColor(sf::Color::Transparent);
        }
        rectangle.setOutlineColor(map_to_sf_color(color));
        // We set the outline thickness to be negative so that the border is
        // rendered inside of the rectangle. That way, we are pixel-accurate
        // with the embedded displays.
        rectangle.setOutlineThickness(-1);

        texture->draw(rectangle);

        /**
         * For border width larger than 1, we let it spill to the outside of the
         * rectangle. This simultates the embedded display behaviour where
         * width 1 is connecting vertices exactly, while larger widths are
         * 'centered' around the actual lines between vertices.
         */
        if (border_width > 1) {
                sf::RectangleShape border({(float)width, (float)height});
                border.setPosition({(float)start.x, (float)start.y});
                border.setFillColor(sf::Color::Transparent);
                border.setOutlineColor(map_to_sf_color(color));
                // We set the outline thickness to be negative so that the
                // border is rendered inside of the rectangle. That way, we are
                // pixel-accurate with the embedded displays.
                border.setOutlineThickness(border_width - 1);
                texture->draw(border);
        }
        texture->display();
};
void SfmlDisplay::draw_rounded_rectangle(Point start, int width, int height,
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
                        draw_circle({.x = x, .y = y}, radius, color, 0, true);
                }
        }

        // This needs to be cleaned up
        draw_rectangle({.x = top_left_corner.x, .y = start.y},
                       width - 2 * radius, height, color, 0, true);

        // small rectangle on the left
        draw_rectangle({.x = start.x, .y = top_left_corner.y}, width + 1,
                       height - 2 * radius, color, 0, true);

        // +1 is because the endY bound is not included
        // The big rectangle in the middle
        draw_rectangle({.x = top_left_corner.x, .y = start.y + height - radius},
                       width - 2 * radius, radius + 1, color, 0, true);
};

void SfmlDisplay::draw_line(Point start, Point end, Color color) const
{
        /**
         * SFML aligns lines to pixel edges. LCD libraries align to the pixel
         * center. We need to add an offset of 0.5 to make the line properly
         * aligned and pixel accurate against the target emulated LCD displays.
         */
        float offset = 0.5;
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(start.x + offset, start.y + offset),
                       map_to_sf_color(color)),
            sf::Vertex(sf::Vector2f(end.x + offset, end.y + offset),
                       map_to_sf_color(color))};

        texture->draw(line, 2, sf::PrimitiveType::Lines);
        texture->display();
}
void SfmlDisplay::draw_string(Point start, char *string_buffer,
                              FontSize font_size, Color bg_color,
                              Color fg_color) const
{
        const sf::Font font = get_emulator_font();
        sf::Text text(font, string_buffer, font_size);

        text.setFillColor(map_to_sf_color(fg_color));
        text.setPosition({(float)start.x, (float)start.y});
        texture->draw(text);
        texture->display();
};
void SfmlDisplay::clear_region(Point top_left, Point bottom_right,
                               Color clear_color) const
{
        draw_rectangle(top_left, bottom_right.x - top_left.x,
                       bottom_right.y - top_left.y, clear_color, 0, true);
};

int SfmlDisplay::get_height() const { return DISPLAY_HEIGHT; }

int SfmlDisplay::get_width() const { return DISPLAY_WIDTH; }

FontConfiguration SfmlDisplay::get_font_configuration() const
{
        return FontConfiguration{
            .font_dimensions = {.width = 10, .height = 16},
            .heading_font_dimensions = {.width = 15, .height = 24}};
}
DisplayDimensions SfmlDisplay::get_display_dimensions() const
{
        return DisplayDimensions{.width = DISPLAY_WIDTH,
                                 .height = DISPLAY_HEIGHT,
                                 .rounded_corner_radius =
                                     DISPLAY_CORNER_RADIUS};
}

int SfmlDisplay::get_display_corner_radius() const { return DISPLAY_CORNER_RADIUS; };

bool SfmlDisplay::refresh() const
{
        /* We need this polling when refreshing the display. Without it, linux
        desktop environments (e.g. gnome) think that the game window is not
        responsive and try to get us to force-close it. */
        while (const std::optional event = window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                        window->close();
                        return false;
                }
        }

        // Now we start rendering to the window, clear it first
        // texture->display();
        window->clear();
        // Draw the texture
        sf::Sprite sprite(texture->getTexture());
        window->draw(sprite);

        // End the current frame and display its contents on screen
        window->display();
        return true;
};

/**
 * The Arduino LCD display uses the RGB565 color encoding, whereas SFML uses
 * RGB888 with the additional opacity channel. This function converts from the
 * RGB565 color to the RGB888 by scaling each channel and setting opacity to 1.
 */
sf::Color map_to_sf_color(uint32_t color)
{
        uint8_t red, green, blue;

        int bitmask_5 = 0b11111;
        int bitmask_6 = 0b111111;

        int original_blue = color & bitmask_5;
        int original_green = (color >> 5) & bitmask_6;
        int original_red = (color >> 11);

        red = (int)((float)original_red / bitmask_5 * 255);
        green = (int)((float)original_green / bitmask_6 * 255);
        blue = (int)((float)original_blue / bitmask_5 * 255);

        return sf::Color(red, green, blue);
}
void SfmlDisplay::drawPixel(int32_t x, int32_t y, uint32_t color) {}
void SfmlDisplay::drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color,
                           uint32_t bg, uint8_t size)
{
}
void SfmlDisplay::drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye,
                           uint32_t color)
{
        sf::Color sf_color = map_to_sf_color(color);
        sf::Vertex line[] = {sf::Vertex(sf::Vector2f(xs, ys), sf_color),
                             sf::Vertex(sf::Vector2f(xe, ye), sf_color)};

        texture->draw(line, 2, sf::PrimitiveType::Lines);
        texture->display();
}
void SfmlDisplay::drawRect(int x, int y, int w, int h, int color)
{
        sf::RectangleShape rectangle({(float)w, (float)h});
        rectangle.setPosition({(float)x, (float)y});
        rectangle.setFillColor(sf::Color::Transparent);
        rectangle.setOutlineColor(map_to_sf_color(color));
        rectangle.setOutlineThickness(-1);
        texture->draw(rectangle);
        texture->display();
}
void SfmlDisplay::fillRect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t color)
{
        sf::RectangleShape rectangle({(float)w, (float)h});
        rectangle.setPosition({(float)x, (float)y});
        sf::Color sf_color = map_to_sf_color(color);
        rectangle.setFillColor(sf_color);
        rectangle.setOutlineColor(sf_color);
        rectangle.setOutlineThickness(-1);
        texture->draw(rectangle);
        texture->display();
}
void SfmlDisplay::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                int32_t radius, uint32_t color)
{
}
void SfmlDisplay::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h,
                                int32_t radius, uint32_t color)
{
}
void SfmlDisplay::drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color)
{
        sf::CircleShape circle(r);
        circle.setPosition({(float)(x - r), (float)(y - r)});
        sf::Color sf_color = map_to_sf_color(color);
        circle.setOutlineColor(sf_color);
        texture->draw(circle);
        texture->display();
}
void SfmlDisplay::fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color)
{
        sf::CircleShape circle(r);
        circle.setPosition({(float)(x - r), (float)(y - r)});
        sf::Color sf_color = map_to_sf_color(color);
        circle.setFillColor(sf_color);
        circle.setOutlineColor(sf_color);
        texture->draw(circle);
        texture->display();
}

void SfmlDisplay::drawEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                              uint32_t color)
{
        sf::CircleShape ellipse(rx);
        ellipse.setPosition({(float)(x - rx), (float)(y - ry)});
        ellipse.setScale({1.0f, (float)ry / rx});
        sf::Color sf_color = map_to_sf_color(color);
        ellipse.setOutlineColor(sf_color);
        ellipse.setOutlineThickness(-1);
        ellipse.setFillColor(sf::Color::Transparent);
        texture->draw(ellipse);
        texture->display();
}
void SfmlDisplay::fillEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                              uint32_t color)
{
        sf::CircleShape ellipse(rx);
        ellipse.setPosition({(float)(x - rx), (float)(y - ry)});
        ellipse.setScale({1.0f, (float)ry / rx});
        sf::Color sf_color = map_to_sf_color(color);
        ellipse.setFillColor(sf_color);
        ellipse.setOutlineColor(sf_color);
        ellipse.setOutlineThickness(-1);
        texture->draw(ellipse);
        texture->display();
}

void SfmlDisplay::drawString(const char *string, int32_t x, int32_t y)
{

        const sf::Font font = get_emulator_font();

        // The default font size is 16. We use the font_size
        // in line with the behaviour of the TFT_eSPI library: font_size == 1
        // means that we are scaling the font as 100%, 2 means 200% and so on.
        // Note that internally our FontSize 16 corresponds to the TFT_eSPI font
        // number 1 and size 2. Because of this, the resolved font size divides
        // the font size by 2 to get our emulated font size back to what
        // corresponds to TFT_eSPI font size 1.
        auto dimensions = get_font_configuration();
        int resolved_font_size =
            dimensions.font_dimensions.height / 2 * this->tft_font_size;
        int resolved_font_width =
            dimensions.font_dimensions.width / 2 * this->tft_font_size;
        sf::Text text(font, string, resolved_font_size);

        // For fonts larger than 1, we need to adjust the vertical alignment of
        // the text to make it pixel-close to the actual TFT_eSPI behaviour.
        float adjustment =
            this->tft_font_size != 1 ? 2 * this->tft_font_size : 1;
        text.setFillColor(sf::Color(this->tft_text_color));
        text.setPosition({(float)(x), (float)(y - adjustment)});
        texture->draw(text);
        texture->display();
}
void SfmlDisplay::fillScreen(uint32_t color) {}
void SfmlDisplay::setTextColor(uint32_t color)
{
        this->tft_text_color = map_to_sf_color(color);
}
void SfmlDisplay::setTextSize(uint8_t size) { this->tft_font_size = size; }

void SfmlDisplay::drawTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                               int32_t xe, int32_t ye, uint32_t color)
{
        sf::ConvexShape triangle;
        triangle.setPointCount(3);
        triangle.setPoint(0, sf::Vector2f(xs, ys));
        triangle.setPoint(1, sf::Vector2f(x2, y2));
        triangle.setPoint(2, sf::Vector2f(xe, ye));
        triangle.setFillColor(sf::Color::Transparent);
        triangle.setOutlineColor(map_to_sf_color(color));
        texture->draw(triangle);
        texture->display();
}
void SfmlDisplay::fillTriangle(int32_t xs, int32_t ys, int32_t x2, int32_t y2,
                               int32_t xe, int32_t ye, uint32_t color)
{
        sf::ConvexShape triangle;
        triangle.setPointCount(3);
        triangle.setPoint(0, sf::Vector2f(xs, ys));
        triangle.setPoint(1, sf::Vector2f(x2, y2));
        triangle.setPoint(2, sf::Vector2f(xe, ye));
        triangle.setFillColor(map_to_sf_color(color));
        triangle.setOutlineColor(map_to_sf_color(color));
        texture->draw(triangle);
        texture->display();
}

void SfmlDisplay::pushImage(int x, int y, int width, int height,
                            const uint16_t *image_array)
{
        sf::Image img({(unsigned int)width, (unsigned int)height});
        for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                        uint16_t pixel = image_array[y * width + x];
                        sf::Color c = map_to_sf_color(pixel);
                        img.setPixel({(unsigned int)x, (unsigned int)y}, c);
                }
        };
        sf::Texture tex;
        bool success = tex.loadFromImage(img);
        if (!success) {
                LOG_ERROR(TAG,
                          "Failed to load texture from image in pushImage");
                return;
        }
        sf::Sprite sprite(tex);
        sprite.setPosition({(float)x, (float)y});
        texture->draw(sprite);
}

TftCompatibleDisplay *SfmlDisplay::cast_into_tft_compatible() { return this; }
#endif
