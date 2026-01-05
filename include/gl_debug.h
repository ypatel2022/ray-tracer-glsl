#pragma once

#include <glad/gl.h>

#include <cstdio>
#include <cstdlib>
#include <string>

/* glGetError wrapper */

static inline void gl_check_error(const char *stmt, const char *file,
                                  int line) {
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error 0x%x at %s:%d for %s\n", err, file, line,
            stmt);
  }
}

#define GL_CALL(stmt)                                                          \
  do {                                                                         \
    stmt;                                                                      \
    gl_check_error(#stmt, __FILE__, __LINE__);                                 \
  } while (0)

/* Shader error checking */

static inline void check_shader(GLuint shader, const char *name) {
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(shader, len, &len, log.data());
    fprintf(stderr, "Shader compile error (%s):\n%s\n", name, log.c_str());
    std::exit(EXIT_FAILURE);
  }
}

static inline void check_program(GLuint program) {
  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(program, len, &len, log.data());
    fprintf(stderr, "Program link error:\n%s\n", log.c_str());
    std::exit(EXIT_FAILURE);
  }
}
