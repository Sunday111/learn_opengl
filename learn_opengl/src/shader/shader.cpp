#include "shader/shader.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

#include "components/type_id_widget.hpp"
#include "nlohmann/json.hpp"
#include "read_file.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "reflection/reflection.hpp"
#include "shader/sampler_uniform.hpp"
#include "shader/shader.hpp"
#include "shader/shader_define.hpp"
#include "shader/shader_uniform.hpp"
#include "template/on_scope_leave.hpp"
#include "texture/texture.hpp"
#include "wrap/wrap_imgui.h"

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
  definitions_initialized_ = false;
  need_recompile_ = false;
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

  if (!definitions_initialized_) {
    if (shader_json.contains("definitions")) {
      for (const auto& def_json : shader_json["definitions"]) {
        defines_.push_back(ShaderDefine::ReadFromJson(def_json));
      }
    }

    definitions_initialized_ = true;
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

  UpdateUniforms();
}

void Shader::DrawDetails() {
  if (ImGui::TreeNode("Static Variables")) {
    for (ShaderDefine& definition : defines_) {
      bool value_changed = false;
      SimpleTypeWidget(definition.type_id, definition.name.GetView(),
                       definition.value.data(), value_changed);

      if (value_changed) {
        need_recompile_ = true;
      }
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Dynamic Variables")) {
    for (ShaderUniform& uniform : uniforms_) {
      bool value_changed = false;
      SimpleTypeWidget(uniform.GetType(), uniform.GetName().GetView(),
                       uniform.GetValue().data(), value_changed);
    }
    ImGui::TreePop();
  }

  if (need_recompile_) {
    Compile();
  }
}

std::optional<UniformHandle> Shader::FindUniform(Name name) const noexcept {
  std::optional<UniformHandle> result;
  for (size_t index = 0; index < uniforms_.size(); ++index) {
    const ShaderUniform& uniform = uniforms_[index];
    if (uniform.GetName() == name) {
      UniformHandle h;
      h.name = name;
      h.index = static_cast<ui32>(index);
      result = h;
      break;
    }
  }

  return result;
}

UniformHandle Shader::GetUniform(Name name) const {
  [[likely]] if (auto maybe_handle = FindUniform(name); maybe_handle) {
    return *maybe_handle;
  }

  throw std::runtime_error(
      fmt::format("Uniform is not found: \"{}\"", name.GetView()));
}

void Shader::SetUniform(UniformHandle& handle, ui32 type_id,
                        std::span<const ui8> value) {
  if (handle.index >= uniforms_.size() ||
      uniforms_[handle.index].GetName() != handle.name) {
    handle = GetUniform(handle.name);
  }

  ShaderUniform& uniform = uniforms_[handle.index];

  [[unlikely]] if (uniform.GetType() != type_id ||
                   uniform.GetValue().size() != value.size()) {
    throw std::runtime_error(
        "Trying to assign a value of invalid type to uniform");
  }

  assert(uniform.GetValue().size() == value.size());
  reflection::TypeHandle type_handle{uniform.GetType()};
  type_handle->copy_assign(uniform.GetValue().data(), value.data());
}

void Shader::SetUniform(UniformHandle& handle,
                        const std::shared_ptr<Texture>& texture) {
  SetUniform(handle, SamplerUniform{texture});
}

void Shader::SendUniforms() {
  for (const ShaderUniform& uniform : uniforms_) {
    uniform.SendValue();
  }
}

void Shader::Check() const {
  [[unlikely]] if (!program_) { throw std::runtime_error("Invalid shader"); }
}

void Shader::Destroy() {
  uniforms_.clear();
  if (program_) {
    glDeleteProgram(*program_);
    program_.reset();
  }
}

std::optional<ui32> ConvertGlType(GLenum gl_type) {
  switch (gl_type) {
    case GL_FLOAT:
      return reflection::GetTypeId<float>();
      break;

    case GL_FLOAT_VEC2:
      return reflection::GetTypeId<glm::vec2>();
      break;

    case GL_FLOAT_VEC3:
      return reflection::GetTypeId<glm::vec3>();
      break;

    case GL_FLOAT_VEC4:
      return reflection::GetTypeId<glm::vec4>();
      break;

    case GL_FLOAT_MAT3:
      return reflection::GetTypeId<glm::mat3>();
      break;

    case GL_FLOAT_MAT4:
      return reflection::GetTypeId<glm::mat4>();
      break;

    case GL_SAMPLER_2D:
      return reflection::GetTypeId<SamplerUniform>();
      break;
  }

  return std::optional<ui32>();
}

void Shader::UpdateUniforms() {
  GLint num_uniforms;
  glGetProgramiv(*program_, GL_ACTIVE_UNIFORMS, &num_uniforms);
  [[unlikely]] if (num_uniforms < 1) { return; }

  GLint max_name_legth;
  glGetProgramiv(*program_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_legth);

  std::string name_buffer_heap;
  constexpr GLsizei name_buffer_size_stack = 64;
  GLchar name_buffer_stack[name_buffer_size_stack];

  GLsizei name_buffer_size;
  char* name_buffer;

  if (max_name_legth < name_buffer_size_stack) {
    name_buffer = name_buffer_stack;
    name_buffer_size = name_buffer_size_stack;
  } else {
    name_buffer_heap.resize(static_cast<size_t>(max_name_legth));
    name_buffer = name_buffer_heap.data();
    name_buffer_size = static_cast<GLsizei>(max_name_legth);
  }

  std::vector<ShaderUniform> uniforms;
  uniforms.reserve(num_uniforms);
  for (GLint i = 0; i < num_uniforms; ++i) {
    GLint variable_size;
    GLenum glsl_type;
    GLsizei actual_name_length;
    glGetActiveUniform(*program_, i, name_buffer_size, &actual_name_length,
                       &variable_size, &glsl_type, name_buffer);

    const std::string_view variable_name_view(
        name_buffer, static_cast<size_t>(actual_name_length));
    const Name variable_name(variable_name_view);

    const std::optional<ui32> cpp_type = ConvertGlType(glsl_type);
    if (!cpp_type) {
      spdlog::warn("Skip variable {} in \"{}\" - unsupported type",
                   variable_name_view, path_.string());
      continue;
    }

    const reflection::TypeHandle type_handle{*cpp_type};

    // find existing variable
    auto found_uniform_it = std::find_if(
        uniforms_.begin(), uniforms_.end(),
        [&](const ShaderUniform& u) { return u.GetName() == variable_name; });

    if (found_uniform_it != uniforms_.end()) {
      uniforms.push_back(std::move(*found_uniform_it));
      // the previous value can be saved only if variable has the same type
      if (*cpp_type != found_uniform_it->GetType()) {
        uniforms.back().SetType(*cpp_type);
      }
    } else {
      uniforms.emplace_back();
      auto& uniform = uniforms.back();
      uniform.SetName(variable_name);
      uniform.SetType(*cpp_type);
    }

    uniforms.back().SetLocation(static_cast<ui32>(i));
  }

  std::swap(uniforms, uniforms_);
}
