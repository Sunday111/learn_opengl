#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "gl_api.hpp"
#include "integer.hpp"
#include "reflection/reflection.hpp"
#include "shader/uniform_handle.hpp"

class ShaderDefine;
class ShaderUniform;
class Texture;

class Shader {
 public:
  Shader(std::filesystem::path path);
  ~Shader();

  void Use();

  void Compile();
  [[nodiscard]] std::optional<ui32> FindUniformLocation(
      const char*) const noexcept;
  [[nodiscard]] ui32 GetUniformLocation(const char*) const noexcept;
  void DrawDetails();

  std::optional<UniformHandle> FindUniform(Name name) const noexcept;
  UniformHandle GetUniform(Name name) const;
  void SetUniform(UniformHandle& handle, ui32 type_id,
                  std::span<const ui8> data);

  void SetUniform(UniformHandle& handle,
                  const std::shared_ptr<Texture>& texture);

  template <typename T>
  void SetUniform(UniformHandle& handle, const T& value) {
    SetUniform(
        handle, reflection::GetTypeId<T>(),
        std::span<const ui8>(reinterpret_cast<const ui8*>(&value), sizeof(T)));
  }

  void SendUniforms();

 protected:
  ShaderUniform& GetUniform(UniformHandle& handle);
  const ShaderUniform& GetUniform(UniformHandle& handle) const;
  std::span<ui8> GetUniformValueViewRaw(UniformHandle& handle, ui32 type_id);
  std::span<const ui8> GetUniformValueViewRaw(UniformHandle& handle,
                                              ui32 type_id) const;
  void UpdateUniformHandle(UniformHandle& handle) const;

  template <typename T>
  T& GetUniformValue(UniformHandle& handle) {
    std::span<ui8> view =
        GetUniformValueViewRaw(handle, reflection::GetTypeId<T>());
    assert(view.size() == sizeof(T));
    return *reinterpret_cast<T*>(view.data());
  }

 private:
  void Check() const;
  void Destroy();
  void UpdateUniforms();

 public:
  static std::filesystem::path shaders_dir_;

 private:
  std::filesystem::path path_;
  std::vector<ShaderDefine> defines_;
  std::vector<ShaderUniform> uniforms_;
  std::optional<GLuint> program_;
  bool definitions_initialized_ : 1;
  bool need_recompile_ : 1;
};
