#if defined(MICROBOX_0)
#include "microbox_v0.hpp"
#elif defined(MICROBOX_1)
#include "microbox_v1.hpp"
#elif defined(MICROBOX_2)
#include "microbox_v2.hpp"
#else
#error "Unrecognized target, please either MICROBOX_1 or MICROBOX_2 build flag."
#endif
