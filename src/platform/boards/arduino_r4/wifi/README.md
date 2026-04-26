# Wi-Fi support

Note that only Arduino R4 Wi-Fi support wifi. Because of this, we
create this board-specific submodule to store all wifi-related implementations.

On Arduino R4 Minima, we don't expose any of the wifi features through the UI and
so it is fine to not initialize those components in the platform.
