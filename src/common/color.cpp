#include "color.hpp"

const char *color_to_string(Color color)
{
        switch (color) {

        case White:
                return "White";
        case Black:
                return "Black";
        case Blue:
                return "Blue";
        case Magenta:
                return "Magenta";
        case GrayBlue:
                return "GrayBlue";
        case Red:
                return "Red";
        case Green:
                return "Green";
        case Cyan:
                return "Cyan";
        case Yellow:
                return "Yellow";
        case Brown:
                return "Brown";
        case BrownRed:
                return "BrownRed";
        case Gray:
                return "Gray";
        case DarkBlue:
                return "DarkBlue";
        case LightBlue:
                return "LightBlue";
        case LightGreen:
                return "LightGreen";
        case LightGray:
                return "LightGray";
        case LightGrayBlue:
                return "GrayishBlue";
        case MediumBlue:
                return "MediumBlue";
        case GreenBlue:
                return "GreenBlue";
        default:
                return "UNKNOWN";
        };
}

/**
 * For a given background color, it returns the text color (Black/White) that
 * will be clearly visible on that background. The idea is to return black for
 * for lighter background colors and default to Black for everything else.
 */
Color get_good_contrast_text_color(Color color)
{

        switch (color) {
        case White:
        case GreenBlue:
        case Green:
        case Cyan:
        case Yellow:
        case LightGray:
                return Black;
        default:
                return White;
        };
}
