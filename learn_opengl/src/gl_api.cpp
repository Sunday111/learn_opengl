#include "gl_api.hpp"

GLuint OpenGl::GenBuffer() {
  GLuint buffer;
  GenBuffers(std::span(&buffer, 1));
  return buffer;
}

void OpenGl::GenBuffers(const std::span<GLuint>& buffers) {
  glGenBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
}

void OpenGl::BindBuffer(GLenum target, GLuint buffer) {
  glBindBuffer(target, buffer);
}
