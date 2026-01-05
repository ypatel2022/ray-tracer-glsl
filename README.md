# GLFW + GLAD + ImGui + GLSL Starter (CMake)

A minimal, batteries included starter project for modern OpenGL development using GLFW, GLAD, Dear ImGui, and GLSL shaders.
Built with a clean CMake setup and designed to be easy to extend for graphics experiments and tooling.

## Features

- GLFW for windowing and input
- GLAD for OpenGL function loading
- Dear ImGui for immediate mode UI
- External GLSL shader support (`.vert`, `.frag`)
- Automatic shader copying at build time
- Cross platform (macOS, Linux, Windows)
- Simple, readable CMake configuration

## Dependencies

- **GLFW** and **ImGui** are included as git submodules
- **GLAD** is generated from https://glad.dav1d.de and vendored into the repo
- OpenGL (system provided)

## Getting the Source

This repo uses submodules. Clone it with:

```bash
git clone https://github.com/ypatel2022/glfw-glad-cmake --recursive
```

## Requirements

- CMake 3.28+
- C++17 compatible compiler
- OpenGL 3.3+ (macOS uses 4.1 core profile)

## Build Instructions

### macOS / Linux

```bash
mkdir build
cd build
cmake ..
cmake --build .
./glfwTestProject
```

### Windows (MinGW)

```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
./glfwTestProject.exe
```

## GLSL Shader Support

- Shaders live in the `shaders/` directory
- Supports `.vert` and `.frag` files
- Shaders are **automatically copied** into the runtime output directory at build time
- No hard coded paths required at runtime

At build time, CMake copies:

```
shaders/*.vert
shaders/*.frag
```

to:

```
<build>/shaders/
```

This allows you to load shaders at runtime using relative paths like:

```
shaders/basic.vert
shaders/basic.frag
```

## Project Structure

```
.
├── src/              # Application source
├── include/          # Headers and utilities
├── shaders/          # GLSL shader sources
├── libs/
│   ├── glfw/         # Submodule
│   ├── imgui/        # Submodule
│   └── glad/         # Vendored GLAD loader
├── CMakeLists.txt
└── README.md
```

## Next Steps

- Add shader hot reloading
- Add texture loading
- Add OpenGL debug callbacks
- Wrap shaders into a small C++ abstraction
# ray-tracer-glsl
