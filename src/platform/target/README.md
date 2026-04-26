# Target Definitions

This directory contains header files that depend on the board- and
driver-specific platform definitions and collect them into target `Platform`
configurations that can be deployed on the corresponding hardware configurations.

Currently there are 3 available target configurations:

- **Microbox 0** - The original prototype configuration running on Arduino R4 Minima,
  1.69 inch LCD display and Arduino input shield. This configuration doesn't support
  wifi, so all wifi-related features are disabled for this build.
- **Microbox 1** - The second version using the same hardware as Microbox 0 but
  powered by Arduino R4 Wi-Fi. This one does have wifi support.
- **Microbox 2** - The third version running on Adafruit esp32 Feather V2,
  2.4 inch LCD display driven by the high-performance TFT_eSPI library and
  controlled by the Adafruit mini gamepad.

The target configuration is resolved by build flags: use
`MICROBOX_<VERSION_NUMBER>` to compile for a given target.
