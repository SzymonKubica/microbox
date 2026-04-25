#pragma once
#include "../interface/platform.hpp"

/**
 * We need to increase the loop stack size on esp32 to run more
 * intensive workloads.
 */

Platform *initialize_platform();
void setup(Platform *platform);
