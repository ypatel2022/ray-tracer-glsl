#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string load_shader_source(const std::string &file_path) {

  std::ifstream file;
  // Ensure ifstream objects can throw exceptions
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  std::string shader_code;

  try {
    file.open(file_path);
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    shader_code = stream.str();
  } catch (std::ifstream::failure e) {
    std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << file_path
              << std::endl;
  }

  return shader_code;
}
