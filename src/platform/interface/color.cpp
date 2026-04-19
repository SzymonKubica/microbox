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
        case Gblue:
                return "Gblue";
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
        case BRRed:
                return "BRRed";
        case Gray:
                return "Gray";
        case DarkBlue:
                return "DarkBlue";
        case LightBlue:
                return "LightBlue";
        case GrayBlue:
                return "GrayBlue";
        case LightGreen:
                return "LightGreen";
        case LGray:
                return "LGray";
        case LGrayBlue:
                return "LGrayBlue";
        case LBBlue:
                return "LBBlue";
        default:
                return "UNKNOWN";
        };
}

Color get_good_contrast_text_color(Color color)
{

        switch (color) {
        case White:
        case Gblue:
        case Green:
        case Cyan:
        case Yellow:
        case LGray:
                return Black;
        default:
                return White;
        };
}
