# MicroBox Platform Definitions

This module contains definitions of interfaces that are required to run the
game console and all of the games. The idea is that the actual physical game
console has an LCD display, physical buttons and a joystick. Additionally the
functionality for sleeping threads is provided by the Arduino `void delay(int
ms)` function.

All other implementations of the platform (e.g. SFML based implementation for
developing games without the physical Arduino) need to implement the same
interface.
