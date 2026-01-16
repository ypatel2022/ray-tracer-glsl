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
  void draw_settings(float &fov, float sun_dir[3], float &sun_intensity,
                     float sun_color[3], float &sky_intensity,
                     float sky_color[3], bool &accumulate_when_still);

  GLFWwindow *window;
  Shader *shader;
  GLuint vao;
  GLuint prev_frame_tex = 0;
  int prev_frame_width = 0;
  int prev_frame_height = 0;
  bool prev_frame_valid = false;

  // Frame time and FPS tracking
  double last_time = 0.0;
  float frame_time = 0.0f;
  float fps = 0.0f;
};
