#if defined(MICROBOX_0)
#pragma once
#include "../interface/platform.hpp"

Platform *initialize_platform();
void setup(Platform *platform);
#endif
