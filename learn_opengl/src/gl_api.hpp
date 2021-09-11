#pragma once

#include <glad/glad.h>

#include <span>

class OpenGl {
 public:
  [[nodiscard]] static GLuint GenVertexArray() noexcept;
  static void GenVertexArrays(const std::span<GLuint>& arrays) noexcept;

  static void BindVertexArray(GLuint array) noexcept;

  [[nodiscard]] static GLuint GenBuffer() noexcept;
  static void GenBuffers(const std::span<GLuint>& buffers) noexcept;

  static void BindBuffer(GLenum target, GLuint buffer) noexcept;

  static void BufferData(GLenum target, GLsizeiptr size, const void* data,
                         GLenum usage) noexcept;

  template <typename T, size_t Extent>
  static void BufferData(GLenum target, const std::span<T, Extent>& data,
                         GLenum usage) noexcept {
    BufferData(target, static_cast<GLsizeiptr>(sizeof(T) * data.size()),
               data.data(), usage);
  }

  template <typename T, size_t Extent>
  static void BufferData(GLenum target, const std::span<const T, Extent>& data,
                         GLenum usage) noexcept;

  [[nodiscard]] static constexpr GLboolean CastBool(bool value) noexcept;

  static void VertexAttribPointer(GLuint index, size_t size, GLenum type,
                                  bool normalized, size_t stride,
                                  const void* pointer) noexcept;

  static void EnableVertexAttribArray(GLuint index) noexcept;

  static void Viewport(GLint x, GLint y, GLsizei width,
                       GLsizei height) noexcept;

  static void SetClearColor(GLfloat red, GLfloat green, GLfloat blue,
                            GLfloat alpha) noexcept;

  static void Clear(GLbitfield mask) noexcept;

  static void UseProgram(GLuint program) noexcept;

  static void DrawElements(GLenum mode, size_t num, GLenum indices_type,
                           const void* indices) noexcept;
};
