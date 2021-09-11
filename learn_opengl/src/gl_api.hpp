#pragma once

#include <glad/glad.h>

#include <span>

class OpenGl {
 public:
  static GLuint GenBuffer();
  static void GenBuffers(const std::span<GLuint>& buffers);
  static void BindBuffer(GLenum target, GLuint buffer);
};