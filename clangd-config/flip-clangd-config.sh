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
# input, it will overwrite the .clangd file with either .clangd-arduino or .clangd-emulator

platform="$1"

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

if [ "$platform" == "arduino" ]; then
  cp .clangd-arduino ../.clangd
else
  cp .clangd-emulator ../.clangd
fi

