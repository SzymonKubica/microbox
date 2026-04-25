# In order to have a decent LSP experience, we need to update the compilation
# database depending on which target we are developing for. The functions below
# use arduino-cli to generate the compile_commands.json that lives under build/
# and clangd uses it to resolve platform-specific imports.
#
function update_compilation_database_arduino_r4_minima() {
set -x
  #  TODO: the code needs to be updated to respect the MICROBOX_0 target
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_1_69_INCH_LCD -DMICROBOX_0 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"  \
    --only-compilation-database
set +x
}
function update_compilation_database_arduino_r4_wifi() {
set -x
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_1_69_INCH_LCD -DMICROBOX_1 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"  \
    --only-compilation-database
set +x
}
function update_compilation_database_esp32() {
set -x
  arduino-cli compile --build-path ./build  \
    --build-property build.extra_flags="-DWAVESHARE_2_4_INCH_LCD -DMICROBOX_2 -DDEBUG_BUILD" \
    --build-property compiler.c.extra_flags="-include Arduino.h"  \
    --only-compilation-database
set +x
}
target="$1"

if [ "$(basename "$PWD")" != "microbox" ]; then
    echo "This script should be executed from the root of the repository."
    exit 1
fi

if [ -z "$target" ]; then
    echo "Usage: $0 <target>"
    echo "target should be either 'V0', 'V1', 'V2', or 'emulator'."
    exit 1
fi

if [ "$target" != "V0" ] && [ "$target" != "V1" ] && [ "$target" != "V2" ] && [ "$target" != "emulator" ]; then
    echo "Invalid target: $target"
    echo "target should be either 'V0', 'V1', 'V2', or 'emulator'."
    exit 1
fi

if [ "$target" == "V0" ]; then
  ./scripts/sketch/flip-config.sh minima
  ./scripts/clangd/flip-config.sh arduino
  echo "Updating compilation database for Arduino R4 Minima..."
  update_compilation_database_arduino_r4_minima
elif [ "$target" == "V1" ]; then
  ./scripts/sketch/flip-config.sh r4wifi
  ./scripts/clangd/flip-config.sh arduino
  echo "Updating compilation database for Arduino R4 WiFi..."
  update_compilation_database_arduino_r4_wifi
elif [ "$target" == "V2" ]; then
  ./scripts/sketch/flip-config.sh esp32
  ./scripts/clangd/flip-config.sh esp32
  echo "Updating compilation database for ESP32..."
  update_compilation_database_esp32
else
  ./scripts/clangd/flip-config.sh emulator
fi
