#include "shader/shader.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include "components/type_id_widget.hpp"
#include "nlohmann/json.hpp"
#include "read_file.hpp"
#include "shader/shader.hpp"
#include "shader/shader_define.hpp"
#include "template/on_scope_leave.hpp"

std::filesystem::path Shader::shaders_dir_;

static void CompileShader(GLuint shader, const std::filesystem::path& path,
                          const std::vector<std::string>& extra_sources) {
  constexpr size_t stack_reserved = 30;

  spdlog::info("compiling shader {}", path.stem().string());
  std::vector<char> buffer;
  ReadFile(path, buffer);

  std::vector<const char*> shader_sources_heap;
  std::vector<GLint> shader_sources_lengths_heap;
  const char* shader_sources_stack[stack_reserved]{};
  GLint shader_sources_lengths_stack[stack_reserved]{};
  const size_t num_sources = extra_sources.size() + 1u;
  const char** shader_sources;
  GLint* shader_sources_lengths;
  if (num_sources > stack_reserved) {
    shader_sources_heap.resize(num_sources);
    shader_sources_lengths_heap.resize(num_sources);
    shader_sources = shader_sources_heap.data();
    shader_sources_lengths = shader_sources_lengths_heap.data();
  } else {
    shader_sources = shader_sources_stack;
    shader_sources_lengths = shader_sources_lengths_stack;
  }

  for (size_t i = 0; i < extra_sources.size(); ++i) {
    shader_sources[i] = extra_sources[i].data();
    shader_sources_lengths[i] = static_cast<GLsizei>(extra_sources[i].size());
  }

  shader_sources[extra_sources.size()] = buffer.data();
  shader_sources_lengths[extra_sources.size()] =
      static_cast<GLsizei>(buffer.size());

  glShaderSource(shader, static_cast<GLsizei>(num_sources), shader_sources,
                 shader_sources_lengths);
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

static GLuint LinkShaders(const std::span<const GLuint>& shaders) {
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

static auto get_shader_json(const std::filesystem::path& path) {
  std::ifstream shader_file(path);
  nlohmann::json shader_json;
  shader_file >> shader_json;
  return shader_json;
}

Shader::Shader(std::filesystem::path path) : path_(std::move(path)) {
  Compile();
}

Shader::~Shader() { Destroy(); }
void Shader::Use() {
  Check();
  OpenGl::UseProgram(*program_);
}

std::optional<ui32> Shader::FindUniformLocation(
    const char* name) const noexcept {
  Check();
  return OpenGl::FindUniformLocation(*program_, name);
}

ui32 Shader::GetUniformLocation(const char* name) const noexcept {
  Check();
  return OpenGl::GetUniformLocation(*program_, name);
}

void Shader::Compile() {
  Destroy();

  nlohmann::json shader_json = get_shader_json(shaders_dir_ / path_);

  size_t num_compiled = 0;
  std::array<GLuint, 2> compiled;
  auto deleter = OnScopeLeave([&]() {
    for (size_t i = 0; i < num_compiled; ++i) {
      glDeleteShader(compiled[i]);
    }
  });

  std::vector<std::string> extra_sources;

  {
    const std::string version = shader_json.at("glsl_version");
    std::string line = fmt::format("#version {}\n\n", version);
    extra_sources.push_back(line);
  }

  if (!initialized_ && shader_json.contains("definitions")) {
    for (const auto& def_json : shader_json["definitions"]) {
      defines_.push_back(ShaderDefine::ReadFromJson(def_json));
    }

    initialized_ = true;
  }

  for (const auto& definition : defines_) {
    extra_sources.push_back(definition.GenDefine());
  }

  auto add_one = [&](GLuint type, const char* json_name) {
    if (!shader_json.contains(json_name)) {
      return;
    }

    auto stage_path = shaders_dir_ / std::string(shader_json[json_name]);
    const GLuint shader = glCreateShader(type);

    try {
      CompileShader(shader, stage_path, extra_sources);
    } catch (...) {
      glDeleteShader(shader);
      throw;
    }

    compiled[num_compiled] = shader;
    num_compiled += 1;
  };

  add_one(GL_VERTEX_SHADER, "vertex");
  add_one(GL_FRAGMENT_SHADER, "fragment");

  program_ = LinkShaders(std::span(compiled).subspan(0, num_compiled));
  need_recompile_ = false;
}

void Shader::DrawDetails() {
  for (ShaderDefine& definition : defines_) {
    bool value_changed = false;
    SimpleTypeWidget(definition.type_id, definition.name,
                     definition.value.data(), value_changed);

    if (value_changed) {
      need_recompile_ = true;
    }
  }

  if (need_recompile_) {
    Compile();
  }
}

void Shader::Check() const {
  [[unlikely]] if (!program_) { throw std::runtime_error("Invalid shader"); }
}

void Shader::Destroy() {
  if (program_) {
    glDeleteProgram(*program_);
    program_.reset();
  }
}

void Shader::PrintUniforms() {
  Check();

  GLint num_uniforms;
  glGetProgramiv(*program_, GL_ACTIVE_UNIFORMS, &num_uniforms);

  GLchar name[100];
  for (GLint i = 0; i < num_uniforms; ++i) {
    GLint strSize;
    GLenum type;
    GLsizei length;
    glGetActiveUniform(*program_, i, sizeof(name), &length, &strSize, &type,
                       name);

    spdlog::warn("Uniform: {}", name);
  }
}
