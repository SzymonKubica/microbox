
echo "[MicroBox] Setting up SFML-based emulator build directory..."
mkdir build-emulator

echo "[MicroBox] Entering build-emulator directory..."
cd build-emulator

echo "[MicroBox] Setting up CMake for SFML emulator..."
cmake ..

echo "[MicroBox] Building SFML-based emulator..."
cmake --build .

echo -e "[MicroBox] SFML-based emulator build complete. You can run the emulator from the \033[1m\033[34m build-emulator \033[0m\033[0m directory."
echo -e "[MicroBox] To run the emulator, use the command: \033[1m\033[32m ./build-emulator/microbox-emulator \033[0m\033[0m"
