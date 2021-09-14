#include "shader/shader_manager.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <fstream>

#include "nlohmann/json.hpp"
#include "read_file.hpp"
#include "shader/shader.hpp"
#include "template/on_scope_leave.hpp"

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

GLuint MakeShader(GLenum type, const std::filesystem::path& path) {
  unsigned int shader = glCreateShader(type);
  CompileShader(shader, path);
  return shader;
}

GLuint LinkShaders(const std::span<const GLuint>& shaders) {
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

std::shared_ptr<Shader> ShaderManager::LoadShader(const std::string& path) {
  if (auto shader_iterator = shaders_.find(path);
      shader_iterator != shaders_.end()) {
    return shader_iterator->second;
  }

  std::ifstream shader_file(shaders_dir_ / path);
  nlohmann::json shader_json;
  shader_file >> shader_json;

  size_t num_compiled = 0;
  std::array<GLuint, 2> compiled;
  auto deleter = OnScopeLeave([&]() {
    for (size_t i = 0; i < num_compiled; ++i) {
      glDeleteShader(compiled[i]);
    }
  });

  auto add_one = [&](GLuint type, const char* json_name) {
    if (shader_json.contains(json_name)) {
      compiled[num_compiled] =
          MakeShader(type, shaders_dir_ / shader_json[json_name]);
      num_compiled += 1;
    }
  };

  add_one(GL_VERTEX_SHADER, "vertex");
  add_one(GL_FRAGMENT_SHADER, "fragment");

  auto shader_ptr = std::make_shared<Shader>();
  shader_ptr->program_ =
      LinkShaders(std::span(compiled).subspan(0, num_compiled));
  shaders_[path] = shader_ptr;
  return shader_ptr;
}

ShaderManager::ShaderManager(const std::filesystem::path shaders_dir)
    : shaders_dir_(shaders_dir) {}

ShaderManager::~ShaderManager() = default;