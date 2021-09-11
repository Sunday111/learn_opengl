#include "gl_api.hpp"

GLuint OpenGl::GenVertexArray() noexcept {
  GLuint array;
  GenVertexArrays(std::span(&array, 1));
  return array;
}
void OpenGl::GenVertexArrays(const std::span<GLuint>& arrays) noexcept {
  glGenVertexArrays(static_cast<GLsizei>(arrays.size()), arrays.data());
}

void OpenGl::BindVertexArray(GLuint array) noexcept {
  glBindVertexArray(array);
}

GLuint OpenGl::GenBuffer() noexcept {
  GLuint buffer;
  GenBuffers(std::span(&buffer, 1));
  return buffer;
}

void OpenGl::GenBuffers(const std::span<GLuint>& buffers) noexcept {
  glGenBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
}

void OpenGl::BindBuffer(GLenum target, GLuint buffer) noexcept {
  glBindBuffer(target, buffer);
}

void OpenGl::BufferData(GLenum target, GLsizeiptr size, const void* data,
                        GLenum usage) noexcept {
  glBufferData(target, size, data, usage);
}

constexpr GLboolean OpenGl::CastBool(bool value) noexcept {
  return static_cast<GLboolean>(value);
}

void OpenGl::VertexAttribPointer(GLuint index, size_t size, GLenum type,
                                 bool normalized, size_t stride,
                                 const void* pointer) noexcept {
  glVertexAttribPointer(index, static_cast<GLint>(size), type,
                        CastBool(normalized), static_cast<GLsizei>(stride),
                        pointer);
}

void OpenGl::EnableVertexAttribArray(GLuint index) noexcept {
  glEnableVertexAttribArray(index);
}

void OpenGl::Viewport(GLint x, GLint y, GLsizei width,
                      GLsizei height) noexcept {
  glViewport(x, y, width, height);
}

void OpenGl::SetClearColor(GLfloat red, GLfloat green, GLfloat blue,
                           GLfloat alpha) noexcept {
  glClearColor(red, green, blue, alpha);
}

void OpenGl::Clear(GLbitfield mask) noexcept { glClear(mask); }

void OpenGl::UseProgram(GLuint program) noexcept { glUseProgram(program); }

void OpenGl::DrawElements(GLenum mode, size_t num, GLenum indices_type,
                          const void* indices) noexcept {
  glDrawElements(mode, static_cast<GLsizei>(num), indices_type, indices);
}