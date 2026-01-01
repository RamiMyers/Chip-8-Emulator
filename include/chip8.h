#ifndef CHIP_8_H
#define CHIP_8_H

#include <iostream>
#include <memory>
#include <array>
#include "screen.h"
#include "buzzer.h"

#define MEMORY 4096
#define DISPLAY_FREQUENCY (float)1 / 120
#define LOG_WIDTH 50

#define Byte unsigned char
#define SignedByte char
#define Word unsigned short

typedef enum { DEBUG_FALSE, DEBUG_TRUE } DebugStates;

class Chip8 {
  private:
    // Memory & Registers
    Byte memory[MEMORY];
    Byte V[16];
    Byte key[16];
    Word I;
    Word opcode;
    void (Chip8::*opcodeTable[16])() = {
      &Chip8::op0xxx,
      &Chip8::op1xxx,
      &Chip8::op2xxx,
      &Chip8::op3xxx,
      &Chip8::op4xxx,
      &Chip8::op5xxx,
      &Chip8::op6xxx,
      &Chip8::op7xxx,
      &Chip8::op8xxx,
      &Chip8::op9xxx,
      &Chip8::opAxxx,
      &Chip8::opBxxx,
      &Chip8::opCxxx,
      &Chip8::opDxxx,
      &Chip8::opExxx,
      &Chip8::opFxxx,
    };

    // Display
    Byte display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    std::unique_ptr<Screen> screen;

    // Sound
    std::unique_ptr<Buzzer> buzzer;

    // State
    Word pc;
    Word stack[16];
    Byte sp;
    Byte debugFlag;
    Byte instructionFrequency;
    SignedByte keyPressed;
    bool paused;

    // Timers
    Byte delayTimer;
    Byte soundTimer;
    float lastTime, currentTime, elapsedTime, deltaTime;

    // Functions
    void Reset();
    void Tick();
    void EmulateCycle();
    void ProcessInput();
    void UpdateTimers();
    void op0xxx();
    void op1xxx();
    void op2xxx();
    void op3xxx();
    void op4xxx();
    void op5xxx();
    void op6xxx();
    void op7xxx();
    void op8xxx();
    void op9xxx();
    void opAxxx();
    void opBxxx();
    void opCxxx();
    void opDxxx();
    void opExxx();
    void opFxxx();

    // Friends
    friend Screen;

  public:
    Chip8(Byte instructionFrequency, Byte debugFlag);
    ~Chip8();
    int LoadROM(const char *romPath);
    void StartMainLoop();
};

#endif
