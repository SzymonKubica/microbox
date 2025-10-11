
echo "[MicroBox] Setting up SFML-based emulator debug build directory..."
mkdir build-emulator-debug

echo "[MicroBox] Entering build-emulator-debug directory..."
cd build-emulator-debug

echo "[MicroBox] Setting up CMake for SFML emulator in debug mode..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "[MicroBox] Building SFML-based emulator with debug symbols & logging..."
cmake --build .

echo -e "[MicroBox] SFML-based emulator build complete. You can run the emulator from the \033[1m\033[34m build-emulator-debug \033[0m\033[0m directory."
echo -e "[MicroBox] To run the emulator, use the command: \033[1m\033[32m ./build-emulator-debug/microbox-emulator \033[0m\033[0m"
echo "[MicroBox] The produced artifact contains debug symbols so you can use gdb on it. The debug logs will now be printed to the console. Happy debugging!"
