
## Project Folder Structure
```
└── src
    ├── common
    ├── config
    ├── fonts
    ├── games
    └── lib
```

- `src` main sources root containing all implementation files and libraries
  required to control the hardware.
- `common` common utilities for creating game menus and reusable UI components..
- `config` contains the header files that specify the pin mappings on the Arduino.
  This is required to ensure that the library code works as expected and is able to
  drive the display. The pin config will depend on how you decide to wire your
  version of the console.
- `fonts` fonts required by the libraries to render characters on the console
  display.
- `games` this folder contains the actual game logic. The idea is that we
  should be able to implement multiple games and reuse the common utilities to create
  UIs.
- `lib` contains the library code that is required to control the LCD display.

## Limitations

The display that I'm currently for the console only allows for redrawing one pixel
at a time. This limits the space of possible games that we can implement.
Anything that requires a large number of pixels to be redrawn between game ticks
is not available. Because of this, this platform is best suited for implementing
text-based games such as 2048 or sudoku.


## :computer: Software Setup

1. Clone this repository:
   ```bash
   git clone https://github.com/<your-username>/MicroBox.git
   cd MicroBox



:hammer_and_wrench: Adding Your Own Games

