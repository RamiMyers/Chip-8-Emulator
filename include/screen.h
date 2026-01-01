#ifndef SCREEN_H
#define SCREEN_H

#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "shader.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define WIDTH 1920
#define HEIGHT 960

class Chip8;

class Screen {
  private:
    GLuint texture;
    GLuint VAO;
    GLuint FBO;
    GLuint RBO;
    GLuint FBOtexture;
    std::vector<unsigned char> *textureData;
    std::unique_ptr<Shader> shader;
    Chip8 *chip8;
    std::vector<std::string> debugLog;

    void MenuBar();
    void Debugger();
    void UpdateTextureData();

  public:
    GLFWwindow *window;

    Screen(const char *vsPath, const char *fsPath, Chip8 *chip8);
    ~Screen();
    void Draw();
    void PushToLog(std::string entry);
};

#endif
