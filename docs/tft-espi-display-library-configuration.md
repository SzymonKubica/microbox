
# Installing TFT_eSPI display library for MicroBox V2 targeting esp32

## Quick Start

1. Install the [`TFT_eSPI`](https://github.com/Bodmer/TFT_eSPI) library.
   The easiest way to achieve this is via Arduino IDE libraries manager.
2. 'Configure' the library by patching the source files.
   The library will be installed under:
    ```
   /home/<your-username>/Arduino/libraries/TFT_eSPI/
    ```
   You need to edit the `User_Setup_Select.h` file and uncomment the line 76
   to select the right configuration preset.
    ```
    #include "User_Setups/Setup42_ILI9341_ESP32.h"           // Setup file for ESP32 and SPI ILI9341 240x320
    ```
    You then need to go into this selected included [setup file](https://github.com/Bodmer/TFT_eSPI/blob/master/User_Setups/Setup42_ILI9341_ESP32.h) and configure
    the pin mapping as follows:
    ```
    #define TFT_MISO 21  // (leave TFT SDO disconnected if other SPI devices share MISO)
    #define TFT_MOSI 19
    #define TFT_SCLK 5
    #define TFT_CS   32  // Chip select control pin
    #define TFT_DC    27  // Data Command control pin
    #define TFT_RST   14  // Reset pin (could connect to RST pin)
    ```
3. You need to ensure that when you connect the wires of the display to your
   esp32 the pin mapping matches the setting configured above.
