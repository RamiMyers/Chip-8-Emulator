#include "chip8.h"
#include "screen.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <time.h>
#include <utilities.h>

int virtualKeys[] = { 
  GLFW_KEY_1, // 0
  GLFW_KEY_2, // 1
  GLFW_KEY_3, // 2
  GLFW_KEY_4, // 3
  GLFW_KEY_Q, // 4
  GLFW_KEY_W, // 5
  GLFW_KEY_E, // 6
  GLFW_KEY_R, // 7
  GLFW_KEY_A, // 8
  GLFW_KEY_S, // 9
  GLFW_KEY_D, // A
  GLFW_KEY_F, // B
  GLFW_KEY_Z, // C
  GLFW_KEY_X, // D
  GLFW_KEY_C, // E
  GLFW_KEY_V, // F
};

Byte fontset[80] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(Byte instructionFrequency, Byte debugFlag) {
  this->instructionFrequency = instructionFrequency;
  this->debugFlag = debugFlag;
  Reset();
  screen = std::make_unique<Screen>("../vertexShader.glsl", "../fragmentShader.glsl", this);
  buzzer = std::make_unique<Buzzer>();
}

void Chip8::Reset() {
  I  = 0;
  pc = 0x200;
  sp = 0;
  delayTimer = 0;
  soundTimer = 0;
  lastTime = 0;
  currentTime = 0;
  elapsedTime = 0;
  deltaTime = 0;
  opcode = 0;
  paused = false;

  srand(time(NULL));
  std::fill(memory, memory + MEMORY, 0);
  std::fill(stack, stack + 16, 0);
  for (int i = 0; i < 80; i++) {
    memory[i] = fontset[i];
  }
  std::fill(display, display + (DISPLAY_WIDTH * DISPLAY_HEIGHT), 0);
}

int Chip8::LoadROM(const char *romPath) {
  Byte *buffer;
  std::size_t bufferSize;
  std::ifstream rom(romPath, std::ios::binary | std::ios::ate);

  Reset();

  if (!rom.is_open())
    return 0;

  bufferSize = rom.tellg();
  buffer = new Byte[bufferSize];

  rom.seekg(0, rom.beg);
  rom.read(reinterpret_cast<char*>(buffer), bufferSize);

  for (int i = 0; i < bufferSize; i++) {
    memory[0x200 + i] = buffer[i];
  }

  rom.close();
  delete[] buffer;

  return 1; 
}

void Chip8::StartMainLoop() {
  Byte soundPlaying = 0;
  while (!glfwWindowShouldClose(screen->window)) {
    screen->Draw();

    if (paused) continue;

    UpdateTimers();

    // Buzzer Control
    if (soundTimer > 0 && !soundPlaying) {
      buzzer->Play();
      soundPlaying = 1;
    }
    else {
      buzzer->Stop();
      soundPlaying = 0;
    }
    
    lastTime = glfwGetTime();

    // Display Refresh
    if (elapsedTime < DISPLAY_FREQUENCY) continue;
    Tick();
    soundTimer = soundTimer > 0 ? soundTimer - 1 : 0;
    delayTimer = delayTimer > 0 ? delayTimer - 1 : 0;
    elapsedTime = 0;
  }
}

void Chip8::UpdateTimers() {
  currentTime = glfwGetTime();
  deltaTime = currentTime - lastTime;
  elapsedTime += deltaTime;
}

void Chip8::Tick() {
  for (int i = 0; i < instructionFrequency; i++) {
    UpdateTimers();
    EmulateCycle();
    lastTime = glfwGetTime();
  }
}

void Chip8::EmulateCycle() {
  opcode = (memory[pc] << 8) | memory[pc + 1];

  // Process input before decoding
  ProcessInput();

  // Decode Instructions
  (this->*opcodeTable[(opcode & 0xF000) >> 12])();
}

void Chip8::ProcessInput() {
  keyPressed = -1;
  for (int i = 0; i < 16; i++) {
    if (glfwGetKey(screen->window, virtualKeys[i]) == GLFW_PRESS) {
      key[i] = 1;
      keyPressed = i;
    } else {
      key[i] = 0;
    }
  }
}

void Chip8::op0xxx() {
  std::stringstream entry;
  switch (opcode) {
    // 0x00E0 - Clear Screen
    case 0x00E0:
      entry << "0x00E0 CLS           |\tClearing Screen";
      std::fill(display, display + (DISPLAY_WIDTH * DISPLAY_HEIGHT), 0);
      pc += 2;
      break;
    // 0x00EE - Return
    case 0x00EE:
      entry << "0x00EE RET           |\t";
      if (sp <= 0) {
        entry << "Stack Underflow! SP = " << int(sp);
        break;
      }
      stack[sp] = 0;
      pc = stack[--sp] + 2;
      entry << "Returning to " << Utilities::FormatHex(3, pc);
      break;
  }
  screen->PushToLog(entry.str());
}

// 0x1nnn - Jump to address nnn
void Chip8::op1xxx() {
  std::stringstream entry;
  pc = opcode & 0x0FFF;
  entry << Utilities::FormatHex(4, opcode) << " JP nnn        |\tSetting PC to: " << Utilities::FormatHex(3, pc);
  screen->PushToLog(entry.str());
}

// 0x2nnn - Call function at nnn
void Chip8::op2xxx() {
  std::stringstream entry;
  entry << Utilities::FormatHex(4, opcode) << " CALL nnn      |\t";
  if (sp >= 16) {
    entry << "Stack Overflow! SP = " << int(sp);
    pc += 2;
    return;
  }
  stack[sp++] = pc;
  pc = opcode & 0x0FFF;
  entry << "Calling function at: " << Utilities::FormatHex(3, pc);
  screen->PushToLog(entry.str());
}

// 0x3xbb - Skip next instruction if V[x] == bb
void Chip8::op3xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  entry << Utilities::FormatHex(4, opcode) << " SE Vx, bb     |\t";
  if (V[x] == (opcode & 0x00FF)) {
    pc += 2;
    entry << "Equal, Skipping";
  } else {
    entry << "Not Equal, Not Skipping";
  }
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0x4xbb - Skip next instruction if V[x] != bb
void Chip8::op4xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  entry << Utilities::FormatHex(4, opcode) << " SNE Vx, bb    |\t";
  if (V[x] != (opcode & 0x00FF)) {
    pc += 2;
    entry << "Not Equal, Skipping";
  } else {
    entry << "Equal, Not Skipping";
  }
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0x5xy0 - Skip next instruction if V[x] == V[y]
void Chip8::op5xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  Byte y = (opcode & 0x00F0) >> 4;
  entry << Utilities::FormatHex(4, opcode) << " SE Vx, Vy     |\t";
  if (V[x] == V[y]) {
    pc += 2;
    entry << "Equal, Skipping";
  } else {
    entry << "Not Equal, Not Skipping";
  }
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0x6xbb - Load bb into V[x]
void Chip8::op6xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  V[x] = opcode & 0x00FF;
  entry << Utilities::FormatHex(4, opcode) << " LD Vx, bb     |\tLoaded " << int(V[x]) << " into V[" << Utilities::FormatHex(1, int(x)) << "]"; 
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0x7xbb - Increment V[x] by bb
void Chip8::op7xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  entry << Utilities::FormatHex(4, opcode) << " ADD Vx, bb    |\tIncrementing V[" << Utilities::FormatHex(1, int(x)) << "] by " << (opcode & 0x00FF); 
  V[x] += opcode & 0x00FF;
  pc += 2;
  screen->PushToLog(entry.str());
}

void Chip8::op8xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  Byte y = (opcode & 0x00F0) >> 4;
  std::string xString = Utilities::FormatHex(1, int(x));
  std::string yString = Utilities::FormatHex(1, int(y));
  std::string opString = Utilities::FormatHex(4, opcode);
  switch (opcode & 0x000F) {
    // 0x8xy0 - Load V[y] into V[x]
    case 0x0000:
      V[x] = V[y];
      entry << opString << " LD Vx, Vy     |\tLoading " << int(V[x]) << " into V[" << xString << "]"; 
      pc += 2;
      break;
    // 0x8xy1 - Set V[x] = V[x] OR V[y]
    case 0x0001:
      V[x] |= V[y];
      entry << opString << " OR Vx, Vy     |\tORing V[" << xString << "] and V[" << yString << "] = " << int(V[x]); 
      pc += 2;
      break;
    // 0x8xy2 - Set V[x] = V[x] AND V[y]
    case 0x0002:
      V[x] &= V[y];
      entry << opString << " AND Vx, Vy    |\tANDing V[" << xString << "] and V[" << yString << "] = " << int(V[x]); 
      pc += 2;
      break;
    // 0x8xy3 - Set V[x] = V[x] XOR V[y]
    case 0x0003:
      V[x] ^= V[y];
      entry << opString << " XOR Vx, Vy    |\tXORing V[" << xString << "] and V[" << yString << "] = " << int(V[x]); 
      pc += 2;
      break;
    // 0x8xy4 - Increment V[x] by V[y]
    case 0x0004: {
      Word sum = V[x] + V[y];
      V[x] = sum & 0xFF;
      if (sum > 0xFF)
        V[0xF] = 1;
      else
        V[0xF] = 0;
      entry << opString << " ADD Vx, Vy    |\tV[" << xString << "] + V[" << yString << "] = " << int(V[x]) << "; V[0xF] = " << int(V[0xF]); 
      pc += 2;
      break;
    }
    // 0x8xy5 - Decrement V[x] by V[y]
    case 0x0005:
      if (V[x] > V[y]) 
        V[0xF] = 1;
      else
        V[0xF] = 0;
      V[x] = V[x] - V[y];
      entry << opString << " SUB Vx, Vy    |\tV[" << xString << "] - V[" << yString << "] = " << int(V[x]) << "; V[0xF] = " << int(V[0xF]); 
      pc += 2;
      break;
    // 0x8xy6 - Shift right V[x] by 1 bit
    case 0x0006:
      V[x] = V[x] >> 1;
      if ((V[x] & 0x01) == 0x01)
        V[0xF] = 1;
      else
        V[0xF] = 0;
      entry << opString << " SHR Vx        |\tV[" << xString << "] >> 1 = " << int(V[x]) << "; V[0xF] = " << int(V[0xF]); 
      pc += 2;
      break;
    // 0x8xy7 - Set V[x] = V[y] - V[x]
    case 0x0007:
      V[x] = V[y] - V[x];
      if (V[y] > V[x])
        V[0xF] = 1;
      else
        V[0xF] = 0;
      entry << opString << " SUBN Vx, Vy   |\tV[" << yString << "] - V[" << xString << "] = " << int(V[x]) << "; V[0xF] = " << int(V[0xF]); 
      pc += 2;
      break;
    // 0x8xyE - Shift left V[x] by 1 bit
    case 0x000E:
      V[x] = V[x] << 1;
      if ((V[x] & 0x80) == 0x80) 
        V[0xF] = 1;
      else
        V[0xF] = 0;
      entry << opString << " SHL Vx        |\tV[" << xString << "] << 1 = " << int(V[x]) << "; V[0xF] = " << int(V[0xF]); 
      pc += 2;
      break;
  }
  screen->PushToLog(entry.str());
}

// 0x9xy0 - Skip next instruction if V[x] != V[y]
void Chip8::op9xxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  Byte y = (opcode & 0x00F0) >> 4;
  entry << Utilities::FormatHex(4, opcode) << " SNE Vx, Vy    |\t";
  if (V[x] != V[y]) {
    pc += 2;
    entry << "Not Equal, Skipping";
  } else {
    entry << "Equal, Not Skipping";
  }
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0xAnnn - Load nnn into I
void Chip8::opAxxx() {
  std::stringstream entry;
  I = opcode & 0x0FFF;
  entry << Utilities::FormatHex(4, opcode) << " LD I, nnn     |\tLoaded " << Utilities::FormatHex(3, I) << " into I";
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0xBnnn - Jump to address nnn + V[0]
void Chip8::opBxxx() {
  std::stringstream entry;
  pc = V[0] + opcode & 0x0FFF;
  entry << Utilities::FormatHex(4, opcode) << " JP V0, addr   |\tSet PC to: " << Utilities::FormatHex(3, pc);
  screen->PushToLog(entry.str());
}

// 0xCxbb - Set V[x] = rand(0, 255) AND bb
void Chip8::opCxxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  std::string xString = Utilities::FormatHex(1, int(x));
  V[x] = (rand() % 256) & (opcode & 0x00FF);
  entry << Utilities::FormatHex(4, opcode) << " RND Vx, bb    |\tSetting V[" << xString << "] to " << int(V[x]);
  pc += 2;
  screen->PushToLog(entry.str());
}

// 0xDxyn - Draw a sprite of n-bytes high at (V[x], V[y])
void Chip8::opDxxx() {
  Byte spriteRow;
  Byte x = V[(opcode & 0x0F00) >> 8] % DISPLAY_WIDTH;
  Byte y = V[(opcode & 0x00F0) >> 4] % DISPLAY_HEIGHT;
  Byte height = opcode & 0x000F;
  std::stringstream entry;
  V[0xF] = 0;
  for (int i = 0; i < height; i++) {
    if (y + i >= DISPLAY_HEIGHT) break;
    if (I + i >= MEMORY) break;
    spriteRow = memory[I + i];
    for (int j = 0; j < 8; j++) {
      if (x + j >= DISPLAY_WIDTH) break;
      Byte pixel = (spriteRow & (0x80 >> j)) > 0 ? 1 : 0;
      if (!pixel) continue;
      unsigned index = x + j + ((y + i) * DISPLAY_WIDTH);
      if (display[index] > 0)
        V[0xF] = 1;
      display[index] ^= pixel;
    }
  }
  entry << Utilities::FormatHex(4, opcode) << " DRW Vx, Vy, n |\tDrawing at (" << int(V[x]) << ", " << int(V[y]) << "), height = " << int(height) << "; V[0xF] = " << int(V[0xF]);
  pc += 2;
  screen->PushToLog(entry.str());
}

void Chip8::opExxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  Byte y = (opcode & 0x00F0) >> 4;
  switch (opcode & 0x00FF) {
    // 0xEx9E - Skip next instruction if the key value of V[x] is pressed
    case 0x009E:
      entry << Utilities::FormatHex(4, opcode) << " SKP Vx        |\t" << Utilities::FormatHex(1, int(V[x])) << " pressed? ";
      if (key[V[x]]) {
        pc += 2;
        entry << "Yes, skipping";
      } else {
        entry << "No, not skipping";
      }
      pc += 2;
      break;
    // 0xExA1 - Skip next instruction if the key value of V[x] is NOT pressed
    case 0x00A1:
      entry << Utilities::FormatHex(4, opcode) << " SKNP Vx       |\t" << Utilities::FormatHex(1, int(V[x])) << " pressed? ";
      if (!key[V[x]]) {
        pc += 2;
        entry << "No, skipping";
      } else {
        entry << "Yes, not skipping";
      }
      pc += 2;
      break;
  }
  screen->PushToLog(entry.str());
}

void Chip8::opFxxx() {
  std::stringstream entry;
  Byte x = (opcode & 0x0F00) >> 8;
  Byte y = (opcode & 0x00F0) >> 4;
  switch (opcode & 0x00FF) {
    // 0xFx07 - Set V[x] = delayTimer
    case 0x0007:
      V[x] = delayTimer;
      pc += 2;
      entry << Utilities::FormatHex(4, opcode) << " LD Vx, DT     |\tSetting V[" << Utilities::FormatHex(1, int(x)) << "] = " << int(delayTimer);
      break;
    // 0xFx0A - Wait for input and store the key value in V[x]
    case 0x000A:
      entry << Utilities::FormatHex(4, opcode) << " LD Vx, K      |\tWating for input... ";
      if (keyPressed < 0) 
        break;
      V[x] = keyPressed;
      entry << "Key " << Utilities::FormatHex(1, V[x]) << " pressed";
      pc += 2;
      break;
    // 0xFx15 - Set delayTimer = V[x]
    case 0x0015:
      delayTimer = V[x];
      pc += 2;
      entry << Utilities::FormatHex(4, opcode) << " LD DT, Vx     |\tSetting Delay Timer = " << int(V[x]);
      break;
    // 0xFx18 - Set soundTimer = V[x]
    case 0x0018:
      soundTimer = V[x];
      pc += 2;
      entry << Utilities::FormatHex(4, opcode) << " LD ST, Vx     |\tSetting Sound Timer = " << int(V[x]);
      break;
    // 0xFx1E - Set I = I + V[x]
    case 0x001E:
      I += V[x];
      pc += 2;
      entry << Utilities::FormatHex(4, opcode) << " ADD I, Vx     |\tI + V[" << Utilities::FormatHex(1, int(x)) << "] = " << Utilities::FormatHex(3, I);
      break;
    // 0xFx29 - Set I equal to the memory address of the font-sprite for the value in V[x]
    case 0x0029:
      I = V[x] * 5;
      pc += 2;
      entry << Utilities::FormatHex(4, opcode) << " LD F, Vx      |\t";
      break;
    // 0xFx33 - Store BCD representation of V[x] at memory locations I, I + 1, I + 2
    case 0x0033:
      memory[I] = V[x] / 100;
      memory[I + 1] = (V[x] % 100) / 10;
      memory[I + 2] = V[x] % 10;
      entry << Utilities::FormatHex(4, opcode) << " LD B, Vx      |\t";
      entry << "memory[" << Utilities::FormatHex(3, I) << "] = "     << int(memory[I])     << "; ";
      entry << "memory[" << Utilities::FormatHex(3, I + 1) << "] = " << int(memory[I + 1]) << "; ";
      entry << "memory[" << Utilities::FormatHex(3, I + 2) << "] = " << int(memory[I + 2]) << "; ";
      pc += 2;
      break;
    // 0xFx55 - Store values from registers V[0] to V[x] into memory[I] onwards
    case 0x0055:
      entry << Utilities::FormatHex(4, opcode) << " LD [I], Vx    |\t";
      for (int i = 0; i <= x && I + i < MEMORY; i++) {
        memory[I + i] = V[i]; 
        entry << "memory[" << Utilities::FormatHex(3, I + i) << "] = " << int(V[i]) << "; ";
      }
      pc += 2;
      break;
    // 0xFx65 - Store values starting from memory[I] into registers V[0] to V[x]
    case 0x0065:
      entry << Utilities::FormatHex(4, opcode) << " LD Vx, [I]    |\t";
      for (int i = 0; i <= x && I + i < MEMORY; i++) {
        V[i] = memory[I + i]; 
        entry << "V[" << Utilities::FormatHex(1, i) << "] = " << int(V[i]) << "; ";
      }
      pc += 2;
      break;
  }
  screen->PushToLog(entry.str());
}

Chip8::~Chip8() {
  std::cout << "Destroying Chip8\n";
}
