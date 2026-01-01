#include "screen.h"
#include "GLFW/glfw3.h"
#include "chip8.h"
#include "utilities.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdarg>
#include <cstring>
#include <ios>
#include <ostream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

void framebufferSizeCallback(GLFWwindow *window, int width, int height);

Screen::Screen(const char *vsPath, const char *fsPath, Chip8 *chip8) {
  GLuint VBO;
  float plane[] = {
    // Vertices   // Texture Coordinates
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
  };
  this->chip8 = chip8;
  textureData = new std::vector<unsigned char>(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);

  // GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Chip8", NULL, NULL);

  if (!window) {
    printf("Window creation failed\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  // GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to load GLAD\n");
    exit(EXIT_FAILURE);
  }

  glViewport(0, 0, WIDTH, HEIGHT);

  // Shader
  shader = std::make_unique<Shader>(vsPath, fsPath);

  // Texture
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  UpdateTextureData();
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData->data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Frame Buffer Object
  glGenFramebuffers(1, &FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);

  // Attaching Texture to FBO
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer creation failed\n";
    std::exit(EXIT_FAILURE);
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  // Vertex Array
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // Vertex Buffer
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

  // Callbacks
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  // ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();
}

void Screen::Draw() {
  glfwPollEvents();

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // Draw to FBO
  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  glBindVertexArray(VAO);
  shader->use();
  shader->setInt("texSample", 0);
  UpdateTextureData();
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, textureData->data());
  glDrawArrays(GL_TRIANGLES, 0, 6);
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) std::cout << "GL Error: " << err << "\n";
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Draw
  glViewport(0, 0, WIDTH, HEIGHT);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  Debugger();
  MenuBar();

  // ImGui Render
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Poll Events & Swap Buffers
  glfwSwapBuffers(window);
}

void Screen::UpdateTextureData() {
  for (unsigned int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
    (*textureData)[i * 4]     = chip8->display[i] * 255;
    (*textureData)[i * 4 + 1] = chip8->display[i] * 255;
    (*textureData)[i * 4 + 2] = chip8->display[i] * 255;
    (*textureData)[i * 4 + 3] = 255;
  }
}

void Screen::MenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::BeginMenu("Open")) {
        fs::path path = "../roms/";
        for (const auto &entry : fs::directory_iterator(path)) {
          std::stringstream filePath;
          std::string fileName = entry.path().filename();
          filePath << path.c_str() << fileName;
          if (ImGui::MenuItem(fileName.c_str())) {
            chip8->LoadROM(filePath.str().c_str()); 
          }
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void Screen::Debugger() {
  // Debugger Settings
  static int steps = 1;
  static int stepCounter = 0;
  static int toggleHex = 1;
  static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
  int freq = static_cast<int>(chip8->instructionFrequency);

  /* Chip8 Screen Window */
  static ImVec2 imageSize(int(WIDTH / 2), int(HEIGHT / 2));
  static ImVec2 screenSize(imageSize.x, imageSize.y + 35);
  ImGui::SetNextWindowPos(ImVec2(WIDTH - screenSize.x, 19));
  ImGui::SetNextWindowSize(screenSize);
  ImGui::Begin("Screen");
  ImGui::Image(texture, imageSize);
  ImGui::End();

  /* Chip8 State Window */
  static ImVec2 stateSize(int(screenSize.x / 2), screenSize.y);
  std::stringstream opcodeStream, pcStream, I_Stream, keyStream, delayStream, soundStream, spStream;
  ImGui::SetNextWindowPos(ImVec2(0.0f, 19.0f));
  ImGui::SetNextWindowSize(stateSize);
  ImGui::Begin("State");
  ImGui::RadioButton("Hex", &toggleHex, 1); ImGui::SameLine();
  ImGui::RadioButton("Decimal", &toggleHex, 0);
  // Initialize Chip8 States as String Streams
  opcodeStream << "Opcode:        " << Utilities::FormatHex(4, chip8->opcode);
  pcStream     << "PC:            ";
  I_Stream     << "I:             ";
  keyStream    << "Key:           ";
  delayStream  << "Delay Timer:   ";
  soundStream  << "Sound Timer:   ";
  spStream     << "Stack Pointer: ";
  // Formats state information depending on user selection
  if (toggleHex) {
    pcStream    << Utilities::FormatHex(3, chip8->pc);
    I_Stream    << Utilities::FormatHex(3, chip8->I);
    keyStream   << Utilities::FormatHex(1, int(chip8->keyPressed));
    delayStream << Utilities::FormatHex(2, int(chip8->delayTimer));
    soundStream << Utilities::FormatHex(2, int(chip8->soundTimer));
    spStream    << Utilities::FormatHex(2, int(chip8->sp));
  }
  else {
    pcStream    << chip8->pc;
    I_Stream    << chip8->I;
    keyStream   << int(chip8->keyPressed);
    delayStream << int(chip8->delayTimer); 
    soundStream << int(chip8->soundTimer); 
    spStream    << int(chip8->sp);
  }
  // Set Key Indicator to NONE if nothing is pressed
  if (chip8->keyPressed == -1) {
    keyStream.str("Key:           NONE");
  }
  // Displays Chip8 State as Formatted Strings
  ImGui::TextUnformatted(pcStream.str().c_str());
  ImGui::TextUnformatted(I_Stream.str().c_str());
  ImGui::TextUnformatted(keyStream.str().c_str());
  ImGui::TextUnformatted(delayStream.str().c_str());
  ImGui::TextUnformatted(soundStream.str().c_str());
  ImGui::TextUnformatted(opcodeStream.str().c_str());
  // Displays V-Registers as a Table
  ImGui::SeparatorText("V-Registers");
  if (ImGui::BeginTable("Registers", 2, tableFlags)) {
    ImGui::TableSetupColumn("Register");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();
    for (int row = 0; row < 16; row ++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("V[%.1X]", row);
      ImGui::TableNextColumn();
      if (toggleHex)
        ImGui::Text("0x%.2X", chip8->V[row]);
      else
        ImGui::Text("%d", chip8->V[row]);
    }
    ImGui::EndTable();
  }
  // Stack
  ImU32 activeColor = ImGui::GetColorU32(ImVec4(0.0f, 0.73f, 1.0f, 0.5f));
  ImGui::TextUnformatted(spStream.str().c_str());
  ImGui::SeparatorText("Stack");
  if (ImGui::BeginTable("Stack", 2, tableFlags)) {
    ImGui::TableSetupColumn("Index");
    ImGui::TableSetupColumn("Function");
    ImGui::TableHeadersRow();
    for (int i = 0; i < 16; i++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if (i == chip8->sp - 1) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, activeColor);
      ImGui::Text("0x%.1X", i);
      ImGui::TableNextColumn();
      if (i == chip8->sp - 1) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, activeColor);
      ImGui::Text("0x%.3X", chip8->stack[i]);
    }
    ImGui::EndTable();
  }
  ImGui::End();

  /* Controls Window */
  static char address[4] = "";
  static int jumpAddress = 0; 
  static bool jumped = false;
  static ImVec2 controlsSize = stateSize;
  ImGui::SetNextWindowPos(ImVec2(screenSize.x - controlsSize.x, 19.0f));
  ImGui::SetNextWindowSize(controlsSize);
  ImGui::Begin("Controls");
  ImGui::PushItemWidth(100.0f);
  // Instruction Frequency Controls
  ImGui::InputInt("Instruction Frequency", &freq);
  chip8->instructionFrequency = freq;
  // Controls for Steps per Button Click
  ImGui::InputInt("Step Count", &steps);
  ImGui::PopItemWidth();
  // Pause Button
  if (ImGui::Button("Pause")) {
    chip8->paused = !chip8->paused;
  }
  // Step Button
  if (ImGui::Button("Step")) {
    if (chip8->paused) {
      for (int i = 0; i < steps; i++) {
        // Ensures that timers are decremented at 60 HZ when paused 
        if (stepCounter % 60 == 0) {
          chip8->soundTimer = chip8->soundTimer > 0 ? chip8->soundTimer - 1 : 0;
          chip8->delayTimer = chip8->delayTimer > 0 ? chip8->delayTimer - 1 : 0;
        }
        chip8->EmulateCycle();
        stepCounter++;
      }
    }
  }
  // Memory Window Input
  ImGui::Text("Jump to Address:"); ImGui::SameLine();
  ImGui::SetItemTooltip("Jumps to an address in the Memory window");
  ImGui::SetNextItemWidth(100.0f);
  if (ImGui::InputTextWithHint("##Address", "<XXX>", address, 4, ImGuiInputTextFlags_EnterReturnsTrue)) {
    jumpAddress = std::stoi(address, 0, 16);
    jumped = true;
  }
  ImGui::SetItemTooltip("Enter a 3-digit hexadecimal address");
  ImGui::End();

  /* Memory Window */
  ImVec2 memorySize = ImVec2(screenSize.x, screenSize.y - 70);
  ImU32 jumpColor = ImGui::GetColorU32(ImVec4(0.0f, 0.73f, 1.0f, 0.1f));
  ImGui::SetNextWindowSize(memorySize);
  ImGui::SetNextWindowPos(ImVec2(0, HEIGHT - memorySize.y));
  ImGui::Begin("Memory");
  // Address Table
  if (ImGui::BeginTable("Memory", 2, tableFlags)) {
    // Moves scrollbar to jumped address (17 was experimentally determined using ImGui::GetScrollY())
    if (jumped) {
      ImGui::SetScrollY(17 * jumpAddress);
      jumped = false;
    }
    ImGui::TableSetupColumn("Address");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();
    for (int i = 0; i < MEMORY; i++) {
      bool cellJumped = i == jumpAddress;
      bool cellActive = i == chip8->pc || i == chip8->pc + 1;
      ImGui::TableNextRow();
      // -- Address
      ImGui::TableNextColumn();
      // Highlight address if it was input
      if (cellJumped) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, jumpColor);
      // Highlight current PC memory location + next address
      if (cellActive) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, activeColor);
      ImGui::Text("0x%.3X", i);
      // -- Value
      ImGui::TableNextColumn();
      if (cellJumped) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, jumpColor);
      if (cellActive) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, activeColor);
      ImGui::Text("0x%.2X", chip8->memory[i]);
    }
    ImGui::EndTable();
  }
  ImGui::End();

  /* Log */
  ImVec2 logSize = memorySize;
  ImGui::SetNextWindowSize(logSize);
  ImGui::SetNextWindowPos(ImVec2(int(WIDTH / 2), HEIGHT - logSize.y));
  ImGui::Begin("Log");
  for (int i = 0; i < debugLog.size(); i++) {
    ImGui::TextUnformatted(debugLog[i].c_str());
  }
  ImGui::End();
}

void Screen::PushToLog(std::string entry) {
  /*std::cout << entry << "\n";*/
  int size = debugLog.size();
  if (size < 100) {
    debugLog.push_back(entry);
  } else {
    std::rotate(debugLog.data(), debugLog.data() + 1, debugLog.data() + size);
    debugLog[size - 1] = entry;
  }
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

Screen::~Screen() {
  delete textureData;
  glDeleteFramebuffers(1, &FBO);
  glDeleteVertexArrays(1, &VAO);
  glDeleteShader(shader->getID());
  glDeleteTextures(1, &texture);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}
