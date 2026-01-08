#pragma once

#include <glad/gl.h>

#include <string>
#include <unordered_map>

class Shader {
public:
  Shader(const std::string &vertex_path, const std::string &fragment_path);
  ~Shader();

  void use() const;
  unsigned int id() const { return program_id; }

  // Uniform helpers
  void set_bool(const std::string &name, bool value) const;
  void set_int(const std::string &name, int value) const;
  void set_float(const std::string &name, float value) const;
  void set_vec2(const std::string &name, float value1, float value2) const;
  void set_vec3(const std::string &name, float value1, float value2,
                float value3) const;

private:
  unsigned int program_id;

  mutable std::unordered_map<std::string, GLint> uniform_cache;
  GLint get_uniform_location(const std::string &name) const;

  static std::string read_file(const std::string &path);
  static unsigned int compile(unsigned int type, const std::string &src);
};
