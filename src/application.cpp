#include "application.h"

#include "gl_debug.h"
#include "shader.h"

#include <cmath>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Application::Application() { initialize(); }

Application::~Application() {
  if (prev_frame_tex != 0) {
    glDeleteTextures(1, &prev_frame_tex);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

struct Camera {
  float position[3] = {0.0f, 0.5f, 3.0f};
  float direction[3] = {0.0f, 0.0f, -1.0f};
  float fov = 45.0f;

  // New: Rotation state
  float yaw = -90.0f;
  float pitch = 0.0f;
};

static bool sun_settings_changed(const float a_dir[3], float a_intensity,
                                 const float a_color[3],
                                 const float b_dir[3], float b_intensity,
                                 const float b_color[3]) {
  const float dir_eps = 1e-4f;
  const float color_eps = 1e-4f;
  const float intensity_eps = 1e-4f;

  for (int i = 0; i < 3; ++i) {
    if (fabsf(a_dir[i] - b_dir[i]) > dir_eps)
      return true;
    if (fabsf(a_color[i] - b_color[i]) > color_eps)
      return true;
  }

  return fabsf(a_intensity - b_intensity) > intensity_eps;
}

static bool sky_settings_changed(float a_intensity, const float a_color[3],
                                 float b_intensity, const float b_color[3]) {
  const float color_eps = 1e-4f;
  const float intensity_eps = 1e-4f;

  for (int i = 0; i < 3; ++i) {
    if (fabsf(a_color[i] - b_color[i]) > color_eps)
      return true;
  }

  return fabsf(a_intensity - b_intensity) > intensity_eps;
}

static bool camera_changed(const Camera &a, const Camera &b) {
  const float pos_eps = 1e-4f;
  const float dir_eps = 1e-4f;
  const float fov_eps = 1e-3f;

  for (int i = 0; i < 3; ++i) {
    if (fabsf(a.position[i] - b.position[i]) > pos_eps)
      return true;
    if (fabsf(a.direction[i] - b.direction[i]) > dir_eps)
      return true;
  }

  return fabsf(a.fov - b.fov) > fov_eps;
}

void update_camera(GLFWwindow *window, Camera &camera, float dt,
                   bool &is_mouse_captured) {
  // 1. Toggle Mouse Capture with TAB
  static bool tab_pressed = false;
  if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
    if (!tab_pressed) {
      is_mouse_captured = !is_mouse_captured;
      glfwSetInputMode(window, GLFW_CURSOR,
                       is_mouse_captured ? GLFW_CURSOR_DISABLED
                                         : GLFW_CURSOR_NORMAL);
      tab_pressed = true;
    }
  } else {
    tab_pressed = false;
  }

  // If mouse isn't captured, don't update camera
  if (!is_mouse_captured)
    return;

  // 2. Mouse Look (Yaw/Pitch)
  static double last_x = 0, last_y = 0;
  static bool first_mouse = true;
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  if (first_mouse) {
    last_x = xpos;
    last_y = ypos;
    first_mouse = false;
  }

  float xoffset = (float)(xpos - last_x);
  float yoffset =
      (float)(last_y -
              ypos); // Reversed since y-coordinates go from bottom to top
  last_x = xpos;
  last_y = ypos;

  float sensitivity = 0.1f;
  camera.yaw += xoffset * sensitivity;
  camera.pitch += yoffset * sensitivity;

  // Clamp pitch to prevent screen flipping
  if (camera.pitch > 89.0f)
    camera.pitch = 89.0f;
  if (camera.pitch < -89.0f)
    camera.pitch = -89.0f;

  // Calculate Direction Vector
  float yaw_rad = camera.yaw * (3.14159f / 180.0f);
  float pitch_rad = camera.pitch * (3.14159f / 180.0f);

  camera.direction[0] = cos(yaw_rad) * cos(pitch_rad);
  camera.direction[1] = sin(pitch_rad);
  camera.direction[2] = sin(yaw_rad) * cos(pitch_rad);

  // Normalize direction (manual math since we don't have GLM here)
  float len = sqrt(camera.direction[0] * camera.direction[0] +
                   camera.direction[1] * camera.direction[1] +
                   camera.direction[2] * camera.direction[2]);
  camera.direction[0] /= len;
  camera.direction[1] /= len;
  camera.direction[2] /= len;

  // 3. Keyboard Movement (WASD)
  float speed = 2.5f * dt;

  // Forward/Backward
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera.position[0] += camera.direction[0] * speed;
    camera.position[1] += camera.direction[1] * speed;
    camera.position[2] += camera.direction[2] * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera.position[0] -= camera.direction[0] * speed;
    camera.position[1] -= camera.direction[1] * speed;
    camera.position[2] -= camera.direction[2] * speed;
  }

  // Strafe Right/Left (Cross Product of Direction and World Up)
  // World Up is (0, 1, 0)
  // Cross((x,y,z), (0,1,0)) = (-z, 0, x)
  float right[3] = {-camera.direction[2], 0.0f, camera.direction[0]};

  // Normalize Right vector
  float r_len = sqrt(right[0] * right[0] + right[2] * right[2]);
  if (r_len > 0) {
    right[0] /= r_len;
    right[2] /= r_len;
  }

  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera.position[0] += right[0] * speed;
    camera.position[1] += right[1] * speed; // usually 0 for walking
    camera.position[2] += right[2] * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera.position[0] -= right[0] * speed;
    camera.position[1] -= right[1] * speed;
    camera.position[2] -= right[2] * speed;
  }

  // Fly Up/Down (Space/Shift)
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    camera.position[1] += speed;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    camera.position[1] -= speed;
  }
}

void Application::run() {

  // Render loop
  int width, height;
  float ratio;

  Camera camera;
  Camera last_camera = camera;
  bool has_last_camera = false;
  unsigned int frame_index = 1;

  float sun_dir[3] = {0.4f, 0.8f, 0.2f};
  float sun_color[3] = {1.0f, 0.95f, 0.85f};
  float sun_intensity = 0.6f;
  float last_sun_dir[3] = {sun_dir[0], sun_dir[1], sun_dir[2]};
  float last_sun_color[3] = {sun_color[0], sun_color[1], sun_color[2]};
  float last_sun_intensity = sun_intensity;
  bool has_last_sun = false;
  float sky_color[3] = {0.5f, 0.7f, 1.0f};
  float sky_intensity = 0.0f;
  float last_sky_color[3] = {sky_color[0], sky_color[1], sky_color[2]};
  float last_sky_intensity = sky_intensity;
  bool has_last_sky = false;

  bool capture_mouse = false; // State to toggle between UI and Look mode
  bool accumulate_when_still = true;

  while (!glfwWindowShouldClose(window)) {
    glfwGetFramebufferSize(window, &width, &height);

    if (width != prev_frame_width || height != prev_frame_height) {
      prev_frame_width = width;
      prev_frame_height = height;
      prev_frame_valid = false;
      frame_index = 1;

      GL_CALL(glBindTexture(GL_TEXTURE_2D, prev_frame_tex));
      GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
                           GL_RGBA, GL_FLOAT, nullptr));
    }

    // Update performance metrics
    update_performance_metrics(last_time, frame_time, fps);

    update_camera(window, camera, frame_time, capture_mouse);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Draw imgui components
    draw_performance_window(fps, frame_time);

    if (!capture_mouse) {
      draw_settings(camera.fov, sun_dir, sun_intensity, sun_color,
                    sky_intensity, sky_color, accumulate_when_still);
    } else {
      // Show a hint
      ImGui::SetNextWindowPos(
          ImVec2(width * 0.5f - 100.0f, (float)height - 50.0f));
      ImGui::Begin("Msg", NULL,
                   ImGuiWindowFlags_NoDecoration |
                       ImGuiWindowFlags_NoBackground);
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Press TAB to release mouse");
      ImGui::End();
    }

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    bool moved = true;
    if (has_last_camera) {
      moved = camera_changed(camera, last_camera);
    }

    bool sun_changed = false;
    if (has_last_sun) {
      sun_changed = sun_settings_changed(
          sun_dir, sun_intensity, sun_color, last_sun_dir, last_sun_intensity,
          last_sun_color);
    }
    bool sky_changed = false;
    if (has_last_sky) {
      sky_changed = sky_settings_changed(sky_intensity, sky_color,
                                         last_sky_intensity, last_sky_color);
    }

    bool disable_still_accum = !accumulate_when_still && !moved;
    bool reset_accum = moved || sun_changed || sky_changed ||
                       !prev_frame_valid || disable_still_accum;
    if (reset_accum) {
      frame_index = 1;
      prev_frame_valid = false;
    } else {
      frame_index += 1;
    }

    shader->use();
    shader->set_float("iTime", (float)glfwGetTime());
    shader->set_vec2("iResolution", (float)width, (float)height);
    shader->set_int("u_frame_index", (int)frame_index);
    shader->set_bool("u_use_prev", !reset_accum && prev_frame_valid);
    shader->set_int("u_prev_frame", 0);
    GL_CALL(glActiveTexture(GL_TEXTURE0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, prev_frame_tex));

    // Pass the updated camera structs
    shader->set_vec3("u_camera.position", camera.position[0],
                     camera.position[1], camera.position[2]);
    shader->set_vec3("u_camera.direction", camera.direction[0],
                     camera.direction[1], camera.direction[2]);
    shader->set_float("u_camera.fov", camera.fov);
    shader->set_vec3("u_sun_direction", sun_dir[0], sun_dir[1], sun_dir[2]);
    shader->set_vec3("u_sun_color", sun_color[0], sun_color[1], sun_color[2]);
    shader->set_float("u_sun_intensity", sun_intensity);
    shader->set_vec3("u_sky_color", sky_color[0], sky_color[1], sky_color[2]);
    shader->set_float("u_sky_intensity", sky_intensity);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    GL_CALL(glBindTexture(GL_TEXTURE_2D, prev_frame_tex));
    GL_CALL(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height));
    prev_frame_valid = true;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();

    last_camera = camera;
    has_last_camera = true;
    for (int i = 0; i < 3; ++i) {
      last_sun_dir[i] = sun_dir[i];
      last_sun_color[i] = sun_color[i];
    }
    last_sun_intensity = sun_intensity;
    has_last_sun = true;
    for (int i = 0; i < 3; ++i) {
      last_sky_color[i] = sky_color[i];
    }
    last_sky_intensity = sky_intensity;
    has_last_sky = true;
  }
}

void Application::initialize() {
  // GLFW Setup
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(1024, 768, "OpenGL Ray Tracer", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);

  gladLoadGL(glfwGetProcAddress);
  glfwSwapInterval(1);

  // ImGui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 410");

  // OpenGL setup
  const float fullscreen_triangle[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};

  GLuint vbo;
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glBindVertexArray(vao));

  GL_CALL(glGenBuffers(1, &vbo));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreen_triangle),
                       fullscreen_triangle, GL_STATIC_DRAW));

  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                                (void *)0));

  // Shader setup
  shader = new Shader("shaders/shader.vert", "shaders/shader.frag");

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  prev_frame_width = width;
  prev_frame_height = height;
  prev_frame_valid = false;

  GL_CALL(glGenTextures(1, &prev_frame_tex));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, prev_frame_tex));
  GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                       GL_FLOAT, nullptr));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

void Application::error_callback(int error, const char *description) {
  fprintf(stderr, "Error: %s\n", description);
}

void Application::key_callback(GLFWwindow *window, int key, int scancode,
                               int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Application::update_performance_metrics(double &last_time,
                                             float &frame_time, float &fps) {
  double current_time = glfwGetTime();
  frame_time = (float)(current_time - last_time);
  fps = 1.0f / frame_time;
  last_time = current_time;
}

void Application::draw_performance_window(float fps, float frame_time) {
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_FirstUseEver);
  ImGui::Begin("Performance", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                   ImGuiWindowFlags_NoTitleBar);

  // Right-aligned numbers with fixed width
  char fps_str[16];
  char frame_time_str[16];
  snprintf(fps_str, sizeof(fps_str), "%6.1f", fps);
  snprintf(frame_time_str, sizeof(frame_time_str), "%6.1f",
           frame_time * 1000.0f);

  ImGui::Text("FPS: %s", fps_str);
  ImGui::Text("Frame Time: %s ms", frame_time_str);
  ImGui::End();
}

void Application::draw_settings(float &fov, float sun_dir[3],
                                float &sun_intensity, float sun_color[3],
                                float &sky_intensity, float sky_color[3],
                                bool &accumulate_when_still) {

  ImGui::Begin("Settings");

  ImGui::SliderFloat("FOV (deg)", &fov, 20, 120);
  ImGui::Separator();
  ImGui::Text("Sun Light");
  ImGui::SliderFloat3("Direction", sun_dir, -1.0f, 1.0f);
  ImGui::SliderFloat("Intensity##Sun", &sun_intensity, 0.0f, 10.0f);
  ImGui::ColorEdit3("Color##Sun", sun_color);
  ImGui::Separator();
  ImGui::Text("Sky Light");
  ImGui::SliderFloat("Intensity##Sky", &sky_intensity, 0.0f, 5.0f);
  ImGui::ColorEdit3("Color##Sky", sky_color);
  ImGui::Separator();
  ImGui::Text("Accumulation");
  ImGui::Checkbox("Accumulate when still", &accumulate_when_still);

  ImGui::End(); 
}
