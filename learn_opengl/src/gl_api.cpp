#include "gl_api.hpp"

#include <fmt/format.h>

#include <stdexcept>

template <typename T>
void GenObjects(T api_fn, const std::span<GLuint>& objects) {
  api_fn(static_cast<GLsizei>(objects.size()), objects.data());
}

template <typename T>
GLuint GenObject(T api_fn) {
  GLuint object;
  GenObjects(api_fn, std::span(&object, 1));
  return object;
}

GLuint OpenGl::GenVertexArray() noexcept {
  return GenObject(glGenVertexArrays);
}

void OpenGl::GenVertexArrays(const std::span<GLuint>& arrays) noexcept {
  GenObjects(glGenVertexArrays, arrays);
}

GLuint OpenGl::GenBuffer() noexcept { return GenObject(glGenBuffers); }

void OpenGl::GenBuffers(const std::span<GLuint>& buffers) noexcept {
  return GenObjects(glGenBuffers, buffers);
}

GLuint OpenGl::GenTexture() noexcept { return GenObject(glGenTextures); }

void OpenGl::GenTextures(const std::span<GLuint>& textures) noexcept {
  GenObjects(glGenTextures, textures);
}

void OpenGl::BindVertexArray(GLuint array) noexcept {
  glBindVertexArray(array);
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
void OpenGl::SetClearColor(const glm::vec4& color) noexcept {
  SetClearColor(color.r, color.g, color.b, color.a);
}

void OpenGl::Clear(GLbitfield mask) noexcept { glClear(mask); }

void OpenGl::UseProgram(GLuint program) noexcept { glUseProgram(program); }

void OpenGl::DrawElements(GLenum mode, size_t num, GLenum indices_type,
                          const void* indices) noexcept {
  glDrawElements(mode, static_cast<GLsizei>(num), indices_type, indices);
}

std::optional<ui32> OpenGl::FindUniformLocation(GLuint shader_program,
                                                const char* name) noexcept {
  int result = glGetUniformLocation(shader_program, name);
  [[likely]] if (result >= 0) return static_cast<ui32>(result);
  return std::optional<ui32>();
}

ui32 OpenGl::GetUniformLocation(GLuint shader_program, const char* name) {
  auto location = FindUniformLocation(shader_program, name);
  [[likely]] if (location.has_value()) return *location;

  throw std::invalid_argument(
      fmt::format("Uniform with name {} was not found", name));
}