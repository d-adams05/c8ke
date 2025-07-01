# c8ke

A modern CHIP-8 emulator with a built-in debugger, using SDL3 and ImGui.

## Features

- Accurate CHIP-8 emulation (timing, sound, instructions)
- Customizable colors for screen and debugger (via ImGui)
- Built-in debugger:
  - Registers, stack, memory viewer
  - Key mapping display
- ROM loader with file dialog support (`.ch8`)
- Beep audio tuning (amount & phase)
- Pause/resume support

## Screenshots

![Screenshot 1](screenshots/screenshot1.png)

![Screenshot 2](screenshots/screenshot2.png)

![Screenshot 3](screenshots/screenshot3.png)

## Credits

- [SDL3](https://github.com/libsdl-org/SDL)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/)

## Extra

There are no ROMs included in this repo, but you can find some here:

- [ROMs for testing the emulator](https://github.com/Timendus/chip8-test-suite)
- [Games and Demos](https://github.com/kripod/chip8-roms)
