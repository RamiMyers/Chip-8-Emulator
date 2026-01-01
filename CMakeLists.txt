# Project Init
cmake_minimum_required(VERSION 3.5)

project(Chip8Emulator)

# Includes
include(FetchContent)

# Settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-std=c++20")
set(CMAKE_BUILD_TYPE Debug)


# Executable
add_executable(${PROJECT_NAME} main.cpp)

# OpenAL
FetchContent_Declare(
  openal-soft
  GIT_REPOSITORY https://github.com/kcat/openal-soft.git
  GIT_TAG        1.24.3
)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)

# X11
if (UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
endif()

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.9-docking
)
FetchContent_MakeAvailable(imgui)
add_library(imgui 
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(imgui PUBLIC glfw)

# Include all fetched libraries
FetchContent_MakeAvailable(openal-soft glfw)

# Local Libraries
include_directories(
  include/ 
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/include/
  ${imgui_SOURCE_DIR}/backends/
)

add_library(Chip8   STATIC src/chip8.cpp)
add_library(Shader  STATIC src/shader.cpp)
add_library(Screen  STATIC src/screen.cpp)
add_library(Buzzer  STATIC src/buzzer.cpp)
add_library(glad    STATIC src/glad.c)

# Compiles OpenGL dependencies to Screen
target_link_libraries(Screen PRIVATE glad glfw GL imgui m Shader)
# Compiles OpenAL dependencies to Buzzer
target_link_libraries(Buzzer PRIVATE openal m)
# Compiles all Chip8 components to the main project
target_link_libraries(${PROJECT_NAME} PRIVATE Chip8 Screen Buzzer)
