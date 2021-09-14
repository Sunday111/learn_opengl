#pragma once

#include "gl_api.hpp"

class Shader {
 public:
  ~Shader();

  void Use();

  GLuint program_;
};