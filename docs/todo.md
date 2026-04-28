# Ideas

- game of life based puzzle game:
  - you have a limited number of seeds
  - you need to clear a given pattern on the grid
  - the pattern is constructed of cells that become cleared whenever at least
    one game of life cell lives there for at least one generation
  - you can pause the evolution and plant a seed whenever you want
  - the fewer evolution generations and seeds you use the better

- android 'emulator':
  - top view of the console
  - actual animated joystick
  - animated buttons
  - somehow reuse the game logic (figure out if it is possible to run c++ on arduino)



# TODO

## Doable on the emulator
- [ ] add aliases for button to have something like 'exit button' instead of Action::BLUE
- [ ] ensure emulator compiles & runs on commodity hardware (e.g. uno q / raspberry pi)
- [ ] document patches required on esp32
- [ ] add 'vendoring' for the TFT LCD display library to ensure users don't need to to
      patching of the library code globally in their arduino libs. This is tricky
      as the library depends on the global arduino installation structure.
      It migth be better to explain / clean-up the process of configuring the
      library with the correct pins and target device.
- [ ] make color enum variants more readable (remove stupid 'LGGBRed' things)
- [ ] add a separate menu for all of the apps.
- [ ] fix state transitions on the light sleep functionality (do we even need
      light sleep anymore?)
- [ ] clean up all usages of raw pointers.
  - [ ] platform code
  - [ ] configuration management code (this is large and messy)
- [ ] add proper way of injecting default wifi ssid & password secrets
- [ ] add game thumbnail rendering
## Require hardware testing
- [ ] optimize sudoku unique solution finding to remove Arduino memory issues.
- [ ] run the full Sudoku uniqueness check on esp32 and skip on arduino (stack size constraints)
- [ ] fix minesweeper crashing
- [ ] ensure that the emulator has pixel precision overrides and not the physical display
- [ ] clean up constants definiton file to remove platform- / display-specific
      overrides
- [ ] add detailed UI rendering for the 2.4 inch display driven by the TFT library
## Dev experience

# In Progress

- [ ] ensure that esp32 doesn't use color coding for the buttons (or 3d print buttons in different colors)
- [ ] reduce duplication in the controls explanations code

# Done
- [x] Feedback from Khemi: users don't care about the color / UI flavour between minimalistic/detailed / hints
      this should be moved to the settings. The main screen should only show the selected game
      and probably feature some cool graphic. The main menu can still remain in its current form
      but should be moved to the settings.
- [x] separate games from applications
- [x] check if there is a vim plugin for markdown rendering.
- [x] fix treesitter crashing stupidly (each documentation lookup fails and treesitter highilghting is broken)
- [x] add config struct version validations
- [x] design a better logging utility to remove the c-style macros. (won't do, macros are good enough)
- [x] clean up all imports to remove the indirect import warnings
- [x] add proper #define switching for esp32
- [x] remove all raw calls to Serial.println and replace those with proper logging.
- [x] restore compatibility with arduino r4 minima
- [x] add dummy wifi provider for the minima saying that there is no wifi and
      then have the code react to that gracefully (e.g. the platform should be
      able to say if it has wifi support and then based on that the code around
      that should react (e.g. wifi app should not display if the microcontroller doesn't have support for the wifi)
- [x] update conditional wiring of the wifi stuff to ensure that it relies on platform
      capabilities instead of #defines.
- [x] clean up display-specific constants and remove the need for build flags
      to specify which display is to be used. (MICROBOX_1/2) flag should be
      sufficient. (this won't be done because display libs are heavyweight and
      they shouldn't all be included if we are only using one display at a time).
- [x] restore compatibility with arduino uno r4
  - [x] wifi library port
  - [x] target wiring
  - [x] display overrides remove
- [x] migrate 1.69 inch display to use the TFT library (imposible as Arduino not supported)
- [x] design new platform separation and ensure that arduino uno r4 wifi/minima are separate from esp32
- [x] separate wifi and http client implementations for arduino uno r4 / esp32
- [x] idea: instead of having 'target' folder with #defines, have the target folder
      construct the ready platform object with all dependencies.
- [x] integrate random seed picker with the new number input
- [x] add validations for the brightness setting
- [x] add number input function
- [x] add persistence and load-on-startup for the brightness settings.
- [x] add brightness changing app
- [x] clean up sudoku before ensuring that solutions are unique
- [x] ensure that generated sudoku grids have unique solutions (have to play a
      bunch of games and verify if the algorithm works fine).
- [x] reenable sudoku grid unique solution testing for esp32
- [x] master zellij and use it for compile/upload/monitor logs workflow.
- [x] fix all new memory leaks
- [x] think about sudoku digit highlighting to make it less intrusive (e.g. small dot instead of underline)
- [x] add small sudoku rendering overrrides to make it look better on the target device
- [x] create devkit controller with the trinkey
- [x] debug sending https requests on arduino (might require changing the web client API) (this is tough)
- [x] add wifi support for esp32
- [x] debug random seed picker
- [x] use a better display library for esp32 (the current AVR-optimized approach is bad).
- [x] remove mandatory wait for input at the end of each application loop
- [x] fix settings loop getting stuck
- [x] design a robust way of telling if a config stored in persistent storage is
      garbage and needs to be thrown away and overwritten
- [x] create a git submodule for the stl assets to make git clone of the main
      microbox a bit more manageable
- [x] tweak instrumentation so that esp32 can also connect to wifi
- [x] change arduino controllers to avoid passing pointers to standard arduino functions.
- [x] add/debug persistent storage on esp32
- [x] add proper deep sleep support
- [x] add proper low power sleep mode
- [x] add sleep mode
- [x] fix sudoku underline rendering
- [x] fix game of life rendering artifacts
- [x] fix integration with the TFT library
- [x] fix integration with the TFT library
- [x] make the game console compile for esp32
- [x] new display & controller testing
   - [x] figure out what is causing issues between the display on SPI and gamepad on I2C.
   - [x] restructure the display lib setup with namespaces to allow the two
         display dimensions to coexist alongside each other
   - [x] figure out how to get the library for the different display dimension
   - [x] wire the display to uno q (decided to do this with the regular r4 as uno q is basically incompatible)
- [x] clean up and simplify state transition propagation handling.
- [x] add extensive documentation of the core control loop executor.
- [x] change terminology from 'game' to 'app' to fit better things like settings,
      wifi config, randomness
- [x] fix exit state transition handling for snake duel
- [x] fix caret rerendering for minesweeper
- [x] ensure that menu state transitions are consistent accross all apps
- [x] create a template for the main game loop as this logic is always the same.
      (or abstract out common functionality to reduce duplication)
- [x] understand the random device stuff (mt19937 etc.)
- [x] ensure that the console starts up correctly even if no stemma qt gamepad connected
- [x] add support for the adafruit stemma qt gamepad controller
- [x] fix 'completed' digits not being highlighted when a sudoku grid is loaded
      from storage
- [x] remove coupling between sudoku engine and view
- [x] clean up sudoku code to remove excessive nesting
- [x] add highlighting of sudoku digits that are already completely placed in
      the digit selector.
- [x] fix minesweeper rendering artifacts
- [x] from 'Ideas'
    - [x] sudoku (assuming we can fit 9x9 grid)
    - [x] figure out how to generate sudoku
    - [x] design the input model for sudoku
    - [x] think about the sudoku game logic
- [x] fix placed cells counting for saved grids.
- [x] add visual cues to highlight numbers that are currently selected
- [x] add ability for saving down the sudoku grid similar to 2048.
- [x] add support for selecting sudoku difficulty level.
- [x] add checking for correctly solved sudoku grids.
- [x] ensure that the sudoku generation algorithm doesn't OOM on arduino.
- [x] implement sudoku randomization given a 'seed' sudoku
- [x] implement sudoku correctness checking
- [x] implement sudoku backtracking solver
- [x] design sudoku input model
- [x] add caret rendering & moves for sudoku
- [x] add sudoku fetching from an API
- [x] add ability to render sudoku cell digits
- [x] implement sudoku grid rendering
- [x] add game app wiring for the sudoku
- [x] debug weird insensitivity to user input in snake duel
- [x] update repo
  - [x] revamp main readme
  - [x] archive the old case & powerbank instructions
  - [x] add new case stl files
  - [x] add pictures of the DIY soldering process.
  - [x] add link to the better power bank
  - [x] explain the minima hack with power bank turning off
  - [x] add instructions to install cmake
- [x] fix logo rendering
- [x] fix game of life rendering artifacts.
- [x] optimize performance of the snake duel bot to reduce lag
- [x] ensure each frame of snake duel takes the same amount of time.
- [x] fix weird snake head rendering on the target device ('flat forehead')
- [x] ensure closing the window actually kills the emulator
- [x] reproduce & fix missing apple bug
- [x] remove hacky pixel-level overrides in the snake code (most likely inaccurate logic)
- [x] ensure that the AI snake can still avoid walls even if it is not able to
      find a path
- [x] add memory efficient point encoding to reduce the global variables size (56%)
      memory is currently occupied
- [x] test whether drawing a single large rectangle is faster than drawing multiple
      small squares. If that is the case, we can optimize game of life rendering by
      looking at contiguous regions of blocks that need to be painted black / white
      and do those using a small number of draw_rectangle commands instead of drawing
      each square separately.
- [x] add transient state where the last selected game stays selected in the game
      menu.
- [x] fix memory errors when running on arduino
- [x] add snake duel config option to enable/disable AI
- [x] add ability to scroll through the config menu for games that require more
      than 3 config options (only if a game requires this, currently not needed)
- [x] fix snake 2 death handling for snake duel
- [x] optimize memory usage of the path finding to prevent crashes on arduino
- [x] fix asan memory leaks on quit
- [x] fix memory leaks in settings page
- [x] fix memory leaks in seed config
- [x] fix memory leaks in Wifi config
- [x] fix memory leaks in Snake
- [x] fix memory leaks in 2048
- [x] fix memory leaks in Minesweeper
- [x] fix memory leaks in Game of Life
- [x] add resource cleanup logic for 'window closed' exceptions raised by the display
- [x] add ability to kill a snake duel game against AI
- [x] fix all ASAN issues in the debug build.
- [x] Add 'AI' functionality to snake duel.
- [x] Add 'AI' functionality prototype.
- [x] final clean-up of snake duel game loop
- [x] remove all duplication between snake and snake duel
- [x] remove wifi game from the minima build
- [x] move the pixel-precision overrides away from the rectangle rendering library code.
- [x] remove pixel-precision adjustments in snake duel
- [x] update the lcd / sfml display libraries to remove the pixel-precision
      discrepancies that require pixel-accurate adjustments (e.g. see line 454 in snake.cpp)
- [x] add instrumentation disabling all wifi stuff on the Arduino R4 minima.
- [x] improve the button debounce handling for physical console (especially for snake). Currently it is
      difficult to pause the game.
- [x] add ability to 'pause' a 2048 game and have it saved in the persistent storage
- [x] for longer string config options add ellipsis rendering
- [x] finish the random seed picker functionality and add ability to overrride the
      seed if there is no wifi support
- [x] figure out why grip depends on having access to the github API.
- [x] fix game of life help prompts disappearing
- [x] update app names to remove 'config' and make it more user-friendly
- [x] make the text input screen more speedy
- [x] add toroidal array to the keyboard input
- [x] add ability to exit out of the text input collection screen
- [x] add help prompt when user tries to save more than 5 wifi configs
- [x] add ability to erase eeprom (probably external sketch)
- [x] add ability to save multiple wifi network settings
- [x] fix segfault when wrapping around a config option value (regression caused by the linked option values refactor)
- [x] make the game of life random grid population truly random (currently it looks
      like the same pattern every time) (The idea is to mess with the seed on input and save
      it in persitent memory)
- [x] ensure that the arduino http-get client still works
- [x] implement the http client interface on the emulator
- [x] move the randomness seed query to the separate game that does not happen
      each time we connect to wifi.
- [x] add generic get requests interface allowing both the emulator and target
      arduino implementation to source the randomness seed.
- [x] add second line text spill for the input keyboard
- [X] add button help prompts for the wifi input screen
- [x] add keyboard interface allowing to insert arbitrary text
- [x] explore using network stack to get true randomness.
- [x] add dummy emulator implementation that would 'connect' to whatever wifi
      the laptop is connected and display that in the network stats.
- [x] add ability to save wifi passwords
- [x] add a 'game' allowing to specify wifi settings and choose if wifi connection
      should be made at startup.
- [x] add platform interface for initializing wifi
- [x] implement snake duel
- [x] Add score counting functionality to Snake
- [x] replace all old instances of 'extract game config'
- [x] add target board switching to allow for compiling for minima
- [x] think about proper copilot keybindings
- [x] update snake game configurations to remove accelerate mode and slow speeds
- [x] add snake game
- [x] update readme and emulator get started instructions, readme revamp.
- [x] ensure that the logo is only rendered on the home screen
- [x] figure out a name for the game
- [x] make a game console logo
- [x] onboard Minesweeper onto the dynamic help hint setup
- [x] make game of life cursor snappy
- [x] implement generic help prompt functionality
- [x] add ability to hide help prompts and have that persisted
- [x] optimize simple help text rendering function to wrap text without relying
      on heavy c++ utils.
- [x] move simple help text rendering function somewhere where it can be reused.
- [x] write help text that fits on the screen
- [x] add ability to create help screens.
- [x] add ability to show help screens
- [x] make the minimalistic 2048 rendering snappy again
- [x] check if it is possible to add colour-coded number rendering for 2048
      (rendering speed needs to be evaluated). -> not possible, even white on black is not snappy enough.
- [x] design a proper minimalistic UI for 2048.
- [x] fix wait for input after quit in each game (this behaviour is unexpected).
- [x] tighten up the exit handling when user tries to break out of gmae of life
      config collection loop. What happens now is that pressing blue throws and
      exception that is not handled.
- [x] add debug logging for arduino (and fix logging code)
- [x] add ability to exit out of 2048.
- [x] design proper consistent UI navigation workflow
- [x] refactor game loops to make adding new games easier
- [x] refactor configuration assembling functions to make adding new games easier
- [x] ensure that debug logs are only compiled in in the debug build mode.
- [x] add smart white/black text switching based on the background.
- [x] add a 'fast-rendering' mode that will skip rendering of the costly UI elements
      (e.g. filled rounded rectangles can be replaced with their borders only)
- [x] clean up color enums to ensure consistent names and remove duplicte enum variants
- [x] figure out how the LCD display encodes colors to create exactly matchign color
      configs for the emulator (this requires access to online documentation).
- [x] fix exception handling functionality (looks like the exception traps the entire
      microcontroller and it freezes)
- [x] test EEPROM saving feature on target device
- [x] fix memory leaks in the main menu
- [x] save high score / default configs in some arduino persistent memory (EEPROM is for that and 8kB
  should be plenty.) Ensure that the config / scores are only written if the user
  explicitly says that the write has to happen. EEPROM has a finite number of write
  cycles so we should not try to write to it in some tight loop.
- [x] debug defaults saving for the emulator.
- [x] define persistent memory interface (first step with templates and then try to add the concept from cpp 20)
- [x] implement emulated persistent memory based on local files
- [x] further optimise game of life implementation to remove memory issues no matter
      how complicated the diff
- [x] fix / find memory leaks in the game of life code / optimize internal representation
      to avoid memory issues. Current hypothesis is that there are no memory leaks.
      The reason is that if we decrease the rewind ring buffer size to 0, the
      game on target device no longer freezes because of memory issues. Also, when
      running the emulator the memory footprint of the console is 148Mb and stays
      like that even if I use the rewind feature.
      Update: there is no memory leaks for sure, the game can handle a rewind buffer
      of 3 evolutions just fine and does not freeze.
- [x] add rewind functionality for game of life
- [x] create macropad setting for neovim gdb plugin
- [x] figure out how to use the graphical gdb showing code snippets
- [x] maybe using gdb in neovim?
- [x] document the gdb workflow
- [x] add the font to the project resources to ensure portability
- [x] add proper non-deterministic randomness for the emulated minesweeper
- [x] fix memory leaks in the game menu
- [x] fix memory leaks in the game of life loop
- [x] add proper deallocation function for the config objects
- [x] fix minesweeper rendering
- [x] implement minesweeper
- [x] design the input model for minesweeper
- [x] refactor 2048 game logic
- [x] finish the generic code for rendering the UI and replace the existing 2048 UI with it
- [x] add platform context object to reduce the number of params for functions
      that have need to interact with the controllers and the display.
      (this was partially done on 2025-07-23 by allowing for an arbitrary length
      list of game controllers).
- [x] find out why the game doesn't restart on the emulator
- [x] move the the 2028 game rendering code to the 2048 files and make it use the
      display interface

- [x] figure out why manual malloc-ing of structs causes segfaults but doing the
      same thing with classes is somehow fine.

      This turned out to be a misdiagnosis. The problem was rooted in the incorrect
      initialization of the game grid array and not in the allocation of the internal state structs

- [x] fix segmentation faults in the emulator

- [x] figure out why the sketch suddenly takes 80% arduino mem (not good - we won't have much space for more games).

      This program size issue was caused by the C++ `<iostream>` header being included for in the arduino build.
      This header was added for debugging in the emulator and it turns out that this library is huge.
      Note that, after removing this header, the memory requirement dropped from 82% to 29% which is
      more than enough what we need.

- [x] find a good way for logging (preferably not compiled in when running on
      Arduino -> including the entire cstdlib is probably the root cause of the
      build image being so large)

      The solution here was to borrow the simple logger from the old weather station
      code. Turns out `printf` doesn't require nearly as much space as `<iostream>` and
      so we can use the logging library without conditionally compiling it only for the
      emulated version.
