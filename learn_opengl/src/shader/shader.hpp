#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "gl_api.hpp"
#include "integer.hpp"

class ShaderDefine;

class Shader {
 public:
  Shader(std::filesystem::path path);
  ~Shader();

  void Use();

  void Compile();
  [[nodiscard]] std::optional<ui32> FindUniformLocation(
      const char*) const noexcept;
  [[nodiscard]] ui32 GetUniformLocation(const char*) const noexcept;
  void PrintUniforms();
  void DrawDetails();

 private:
  void Check() const;
  void Destroy();

 public:
  static std::filesystem::path shaders_dir_;

 private:
  std::filesystem::path path_;
  std::vector<ShaderDefine> defines_;
  std::optional<GLuint> program_;
  bool initialized_ = false;
  bool need_recompile_ = false;
};
