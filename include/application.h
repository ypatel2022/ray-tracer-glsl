#pragma once

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "shader.h"

class Application {
public:
  Application();
  ~Application();

  void run();

private:
  void initialize();

  static void error_callback(int error, const char *description);

  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods);

  void update_performance_metrics(double &last_time, float &frame_time,
                                  float &fps);

  void draw_performance_window(float fps, float frame_time);
  void draw_settings(float &fov);

  GLFWwindow *window;
  Shader *shader;
  GLuint vao;

  // Frame time and FPS tracking
  double last_time = 0.0;
  float frame_time = 0.0f;
  float fps = 0.0f;
};
