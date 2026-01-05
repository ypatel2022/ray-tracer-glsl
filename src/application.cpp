#include "application.h"

#include "gl_debug.h"
#include "shader.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Application::Application() { initialize(); }

Application::~Application() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

void Application::run() {

  // Render loop
  int width, height;
  float ratio;

  float fov = 45.0f;

  while (!glfwWindowShouldClose(window)) {
    // Update performance metrics
    update_performance_metrics(last_time, frame_time, fps);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Draw imgui components
    draw_performance_window(fps, frame_time);
    draw_settings(fov);

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->use();
    shader->set_float("iTime", (float)glfwGetTime());
    shader->set_2float("iResolution", (float)width, (float)height);
    shader->set_float("fov", fov);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
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

  window = glfwCreateWindow(1024, 768, "OpenGL Triangle", NULL, NULL);
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
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch

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
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar);

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


void Application::draw_settings(float &fov) {

    // 2. Build your UI hierarchy using ImGui functions
    ImGui::Begin("My First Window"); // Start a new window
    ImGui::Text("Hello, world!");     // Add text


    ImGui::SliderFloat("FOV (deg)", &fov, 20, 120);
    if (ImGui::Button("Click Me")) {  // Add a button
        // Handle button click logic here
    }
    ImGui::End(); // End the window


}
