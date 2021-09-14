#pragma once

#include <optional>
#include <string_view>

#include "gl_api.hpp"

class ShaderManager;

class Shader {
 public:
  Shader();
  ~Shader();

  void Use();

  [[nodiscard]] std::optional<ui32> FindUniformLocation(
      const char*) const noexcept;
  [[nodiscard]] ui32 GetUniformLocation(const char*) const noexcept;

 private:
  friend ShaderManager;
  GLuint program_;
};