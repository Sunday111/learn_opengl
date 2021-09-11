#pragma once

#include <array>
#include <filesystem>
#include <vector>

#include "gl_api.hpp"

void CompileShader(GLuint shader, const std::filesystem::path& path) {
  spdlog::info("compiling shader {}", path.stem().string());
  std::vector<char> buffer;
  ReadFile(path, buffer);
  const char* shader_sources = buffer.data();
  const GLint shader_sources_lengths = static_cast<GLint>(buffer.size());
  glShaderSource(shader, 1, &shader_sources, &shader_sources_lengths);
  glCompileShader(shader);

  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  [[unlikely]] if (!success) {
    GLint info_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
    std::string error_info;
    if (info_length > 0) {
      error_info.resize(static_cast<size_t>(info_length));
      glGetShaderInfoLog(shader, info_length, NULL, error_info.data());
    }
    throw std::runtime_error(fmt::format("failed to compile shader {} log:\n{}",
                                         path.stem().string(), error_info));
  }
}

[[nodiscard]] GLuint MakeShader(GLenum type,
                                const std::filesystem::path& path) {
  unsigned int shader = glCreateShader(type);
  CompileShader(shader, path);
  return shader;
}

[[nodiscard]] GLuint LinkShaders(const std::span<const GLuint>& shaders) {
  GLuint program = glCreateProgram();
  for (auto shader : shaders) {
    glAttachShader(program, shader);
  }

  glLinkProgram(program);
  int success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  [[likely]] if (success) { return program; }

  GLint info_length;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_length);
  std::string error_info;
  if (info_length > 0) {
    error_info.resize(static_cast<size_t>(info_length));
    glGetShaderInfoLog(program, info_length, NULL, error_info.data());
  }
  glDeleteProgram(program);
  throw std::runtime_error(
      fmt::format("failed to link shaders. Log:\n{}", error_info));
}

struct ShaderProgramInfo {
  std::filesystem::path vertex;
  std::filesystem::path fragment;
};

template <typename Function>
class OnScopeLeaveHandler {
 public:
  OnScopeLeaveHandler(Function function) : function_(std::move(function)) {}
  ~OnScopeLeaveHandler() { function_(); }

 private:
  Function function_;
};

template <typename Function>
auto OnScopeLeave(Function&& fn) {
  return OnScopeLeaveHandler{std::forward<Function>(fn)};
}

[[nodiscard]] GLuint MakeShaderProgram(const ShaderProgramInfo& info) {
  GLuint vertex_shader;
  GLuint fragment_shader;

  auto scope_guard = OnScopeLeave([&]() {
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  });

  vertex_shader = MakeShader(GL_VERTEX_SHADER, info.vertex);
  fragment_shader = MakeShader(GL_FRAGMENT_SHADER, info.fragment);

  return LinkShaders(std::array{vertex_shader, fragment_shader});
}