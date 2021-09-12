#pragma once

#include <glad/glad.h>

#include <array>
#include <bit>
#include <optional>
#include <span>

#include "include_glm.hpp"
#include "integer.hpp"

enum class GlPolygonMode : ui8 { Point, Line, Fill, Max };

class OpenGl {
 public:
  [[nodiscard]] static GLuint GenVertexArray() noexcept;
  static void GenVertexArrays(const std::span<GLuint>& arrays) noexcept;

  static void BindVertexArray(GLuint array) noexcept;

  [[nodiscard]] static GLuint GenBuffer() noexcept;
  static void GenBuffers(const std::span<GLuint>& buffers) noexcept;

  [[nodiscard]] static GLuint GenTexture() noexcept;
  static void GenTextures(const std::span<GLuint>& textures) noexcept;

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
                         GLenum usage) noexcept {
    BufferData(target, static_cast<GLsizeiptr>(sizeof(T) * data.size()),
               data.data(), usage);
  }

  [[nodiscard]] static constexpr GLboolean CastBool(bool value) noexcept;

  static void VertexAttribPointer(GLuint index, size_t size, GLenum type,
                                  bool normalized, size_t stride,
                                  const void* pointer) noexcept;

  static void EnableVertexAttribArray(GLuint index) noexcept;

  static void Viewport(GLint x, GLint y, GLsizei width,
                       GLsizei height) noexcept;

  static void SetClearColor(GLfloat red, GLfloat green, GLfloat blue,
                            GLfloat alpha) noexcept;
  static void SetClearColor(const glm::vec4& color) noexcept;

  static void Clear(GLbitfield mask) noexcept;

  static void UseProgram(GLuint program) noexcept;

  static void DrawElements(GLenum mode, size_t num, GLenum indices_type,
                           const void* indices) noexcept;

  [[nodiscard]] constexpr static GLenum ConvertPolygonMode(
      GlPolygonMode mode) noexcept {
    static_assert(std::underlying_type_t<GlPolygonMode>(GlPolygonMode::Max) ==
                  3);
    switch (mode) {
      case GlPolygonMode::Point:
        return GL_POINT;
        break;

      case GlPolygonMode::Line:
        return GL_LINE;
        break;

      default:
        return GL_FILL;
        break;
    }
  }

  static void SetUniform(ui32 location, const glm::vec4& v) noexcept {
    glUniform4f(static_cast<GLint>(location), v.r, v.g, v.b, v.a);
  }

  static void SetTextureParameter(GLenum target, GLenum pname,
                                  const GLfloat* value) noexcept {
    glTexParameterfv(target, pname, value);
  }

  static void SetTexture2dBorderColor(GLenum target,
                                      const glm::vec4& v) noexcept {
    SetTextureParameter(target, GL_TEXTURE_BORDER_COLOR,
                        reinterpret_cast<const float*>(&v));
  }

  static void BindTexture(GLenum target, GLuint texture) noexcept {
    glBindTexture(target, texture);
  }

  static void BindTexture2d(GLuint texture) {
    BindTexture(GL_TEXTURE_2D, texture);
  }

  static void TexImage2d(GLenum target, size_t level_of_detail,
                         GLint internal_format, size_t width, size_t height,
                         GLenum data_format, GLenum pixel_data_type,
                         const void* pixels) noexcept {
    glTexImage2D(target, static_cast<GLint>(level_of_detail), internal_format,
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                 data_format, pixel_data_type, pixels);
  }

  static void GenerateMipmap(GLenum target) noexcept {
    glGenerateMipmap(target);
  }

  static void GenerateMipmap2d() noexcept { GenerateMipmap(GL_TEXTURE_2D); }

  [[nodiscard]] static std::optional<ui32> FindUniformLocation(
      GLuint shader_program, const char* name) noexcept;

  [[nodiscard]] static ui32 GetUniformLocation(GLuint shader_program,
                                               const char* name);

  static void PolygonMode(GLenum mode) noexcept {
    glPolygonMode(GL_FRONT_AND_BACK, mode);
  }

  static void PolygonMode(GlPolygonMode mode) noexcept {
    const GLenum converted = ConvertPolygonMode(mode);
    glPolygonMode(GL_FRONT_AND_BACK, converted);
  }

  static void PointSize(float size) noexcept { glPointSize(size); }
  static void LineWidth(float width) noexcept { glLineWidth(width); }
};
