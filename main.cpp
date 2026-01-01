// External Libraries
#include "chip8.h"

int main(int argc, char **argv) {
  // Chip8
  Chip8 chip8(16, 0);
  chip8.LoadROM("../roms/chip8Logo.ch8");
  chip8.StartMainLoop();

  return 0;
}
