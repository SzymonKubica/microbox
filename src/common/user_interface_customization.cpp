#include "user_interface_customization.hpp"
#include <cstring>

const char *rendering_mode_to_str(UserInterfaceRenderingMode mode)
{
        switch (mode) {
        case Minimalistic:
                return "Minimalistic";
        case Detailed:
                return "Detailed";
        }
        return "UNKNOWN";
}

UserInterfaceRenderingMode rendering_mode_from_str(const char *mode_str)
{
        if (strcmp(mode_str, rendering_mode_to_str(Minimalistic)) == 0) {
                return UserInterfaceRenderingMode::Minimalistic;
        }
        return UserInterfaceRenderingMode::Detailed;
}
