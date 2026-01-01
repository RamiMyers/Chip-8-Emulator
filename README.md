# Chip-8 Emulator

A Chip-8 emulator and debugger written in C++. The project implements a full fetch-decode-execute cycle, instruction decoding, timers, stack and memory management, and an interactive debugger for step-by-step execution and state inspection.

Rendering is handled via OpenGL, with ImGui used to implement the debugging interface.

## Features
- Full Chip-8 CPU emulation (fetch, decode, execute)
- Instruction decoding and execution
- Memory, register file, stack, and timer management
- Interactive debugger for stepping through execution and inspecting state
- Spec-driven implementation focused on correctness and determinism

## Dependencies
This project uses **CMake FetchContent** to automatically download and build its dependencies from their official GitHub repositories.

Dependencies include:
- OpenGL
- GLFW
- GLAD
- ImGui
- OpenAL

> Note: Because dependencies are built from source during the first build, initial compilation can take several minutes.

## Building (Ubuntu)

From the repository root:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Running

- After running the exeuctable, you will be asked to insert a ROM.
- ROMs are located in the `roms` directory. You can add your own, or use the ones that come with this demo.
- To insert a ROM, enter the name of the ROM file, minus the `.ch8` extension.
