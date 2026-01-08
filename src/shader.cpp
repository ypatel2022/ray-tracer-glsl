#include "shader.h"

#include "gl_debug.h"
#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>

static void checkCompileErrors(unsigned int shader, const std::string &type) {
  int success;
  char infoLog[1024];

  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
      std::cerr << "[Shader] Compile error (" << type << "):\n"
                << infoLog << std::endl;
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
      std::cerr << "[Shader] Link error:\n" << infoLog << std::endl;
    }
  }
}

Shader::Shader(const std::string &vertex_path,
               const std::string &fragment_path) {

  std::string vert_code = read_file(vertex_path);
  std::string frag_code = read_file(fragment_path);

  unsigned int vert = compile(GL_VERTEX_SHADER, vert_code);
  unsigned int frag = compile(GL_FRAGMENT_SHADER, frag_code);

  program_id = glCreateProgram();
  glAttachShader(program_id, vert);
  glAttachShader(program_id, frag);
  glLinkProgram(program_id);
  check_program(program_id);

  checkCompileErrors(program_id, "PROGRAM");

  glDeleteShader(vert);
  glDeleteShader(frag);
}

Shader::~Shader() { glDeleteProgram(program_id); }

void Shader::use() const { glUseProgram(program_id); }

std::string Shader::read_file(const std::string &path) {
  std::ifstream file(path);
  std::stringstream buffer;

  if (!file.is_open()) {
    std::cerr << "[Shader] Failed to open file: " << path << std::endl;
    return "";
  }

  buffer << file.rdbuf();
  return buffer.str();
}

unsigned int Shader::compile(unsigned int type, const std::string &src) {
  const char *code = src.c_str();

  unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &code, nullptr);
  glCompileShader(shader);

  checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");

  return shader;
}

void Shader::set_bool(const std::string &name, bool value) const {
  glUniform1i(get_uniform_location(name), (int)value);
}

void Shader::set_int(const std::string &name, int value) const {
  glUniform1i(get_uniform_location(name), value);
}

void Shader::set_float(const std::string &name, float value) const {
  glUniform1f(get_uniform_location(name), value);
}

void Shader::set_vec2(const std::string &name, float value1,
                      float value2) const {
  glUniform2f(get_uniform_location(name), value1, value2);
}

void Shader::set_vec3(const std::string &name, float value1, float value2,
                      float value3) const {
  glUniform3f(get_uniform_location(name), value1, value2, value3);
}

GLint Shader::get_uniform_location(const std::string &name) const {
  auto it = uniform_cache.find(name);
  if (it != uniform_cache.end())
    return it->second;

  GLint location = glGetUniformLocation(program_id, name.c_str());
  uniform_cache[name] = location;

#ifdef DEBUG
  std::cout << "DEBUG MODE" << std::endl;
  if (location == -1) {
    std::cerr << "[Shader] Warning: uniform '" << name
              << "' not found or optimized out\n";
  }
#endif

  return location;
}
