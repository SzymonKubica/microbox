# Compile-Target script allows for quickly building the project for a specific
# target configuration. It makes necessary adjsutments to the sketch config
# and then performs the compilation.

function compile_arduino_r4_minima() {
set -x
  #  TODO: the code needs to be updated to respect the MICROBOX_0 target
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_1_69_INCH_LCD -DMICROBOX_0 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"
set +x
}
function compile_arduino_r4_wifi() {
set -x
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_1_69_INCH_LCD -DMICROBOX_1 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"
set +x
}
function compile_esp32() {
set -x
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_2_4_INCH_LCD -DMICROBOX_2 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"
set +x
}
target="$1"

if [ "$(basename "$PWD")" != "microbox" ]; then
    echo "This script should be executed from the root of the repository."
    exit 1
fi

if [ -z "$target" ]; then
    echo "Usage: $0 <target>"
    echo "compilation target should be either 'V0', 'V1', 'V2'."
    exit 1
fi

if [ "$target" != "V0" ] && [ "$target" != "V1" ] && [ "$target" != "V2" ]; then
    echo "Invalid target: $target"
    echo "compilation target should be either 'V0', 'V1', 'V2'."
    exit 1
fi

# Before compiling, we need to ensure that the sketch config is set to the
# correct target.

if [ "$target" == "V0" ]; then
  ./scripts/sketch/flip-config.sh minima
  compile_arduino_r4_minima
elif [ "$target" == "V1" ]; then
  ./scripts/sketch/flip-config.sh r4wifi
  compile_arduino_r4_wifi
elif [ "$target" == "V2" ]; then
  ./scripts/sketch/flip-config.sh esp32
  compile_esp32
else
  echo "Unreachable"
fi
