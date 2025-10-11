# This script allows for ensuring that the neovim LSP based on clangd works
# with the project both when we are developing native arduino code and the
# emulator code. The thing is that the library dependencies for those two use
# cases are fundamentally incompatible.
#
# Because of this, we need to tell clangd which compile commands file is to be
# used. This is done through the .clangd configuration file. The problem is that
# clangd does not allow specifing which .clangd file is to be soruced, so if
# we want to change the configuration conditionally depending on which platform
# we want to target for development, we need to actually overwrite the .clangd
# file with the desired settings.
#
# This script accepts the name of the platform (arduino/emulator). Based on this
# input, it will overwrite the .clangd file int the root of the repo with either .clangd-arduino or .clangd-emulator
#
# Note: while should be executed from the root of the repository.

platform="$1"

if [ "$(basename "$PWD")" != "microbox" ]; then
    echo "This script should be executed from the root of the repository."
    exit 1
fi

if [ -z "$platform" ]; then
    echo "Usage: $0 <platform>"
    echo "Platform should be either 'arduino' or 'emulator'."
    exit 1
fi

if [ "$platform" != "arduino" ] && [ "$platform" != "emulator" ]; then
    echo "Invalid platform: $platform"
    echo "Platform should be either 'arduino' or 'emulator'."
    exit 1
fi

set -x # Enables command echo-ing
if [ "$platform" == "arduino" ]; then
  cp scripts/clangd/clangd-arduino .clangd
else
  cp scripts/clangd/clangd-emulator .clangd
fi
set -x
