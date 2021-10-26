#include "shader/shader_uniform.hpp"

#include <stdexcept>

#include "fmt/format.h"
#include "gl_api.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "sampler_uniform.hpp"

template <typename T>
struct ValueTypeHelper {
  static bool Exec(ui32 type_id, ui32 location, std::span<const ui8> value) {
    if (type_id == reflection::GetTypeId<T>()) {
      OpenGl::SetUniform(location, *reinterpret_cast<const T*>(value.data()));
      return true;
    }

    return false;
  }
};

template <>
struct ValueTypeHelper<SamplerUniform> {
  static bool Exec(ui32 type_id, [[maybe_unused]] ui32 location,
                   std::span<const ui8> value) {
    if (type_id == reflection::GetTypeId<SamplerUniform>()) {
      auto& v = *reinterpret_cast<const SamplerUniform*>(value.data());
      const auto texture_handle = v.texture->GetHandle();

      static_assert(GL_TEXTURE31 - GL_TEXTURE0 == 31);
      glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + v.sampler_index));
      glBindTexture(GL_TEXTURE_2D, texture_handle);
      glUniform1i(location, v.sampler_index);

      return true;
    }

    return false;
  }
};

template <typename T>
bool SendActualValue(ui32 type_id, ui32 location, std::span<const ui8> value) {
  return ValueTypeHelper<T>::Exec(type_id, location, value);
}

ShaderUniform::ShaderUniform() = default;

ShaderUniform::~ShaderUniform() { Clear(); }

ShaderUniform::ShaderUniform(ShaderUniform&& another) { MoveFrom(another); }

void ShaderUniform::MoveFrom(ShaderUniform& another) {
  Clear();
  name_ = std::move(another.name_);
  value_ = std::move(another.value_);
  location_ = another.location_;
  type_id_ = another.type_id_;
  sent_ = another.sent_;
}

void ShaderUniform::SendValue() const {
  CheckNotEmpty();

  if (sent_) {
    return;
  }

  sent_ = true;

  const bool type_found =
      SendActualValue<float>(type_id_, location_, value_) ||
      SendActualValue<glm::vec2>(type_id_, location_, value_) ||
      SendActualValue<glm::vec3>(type_id_, location_, value_) ||
      SendActualValue<glm::vec4>(type_id_, location_, value_) ||
      SendActualValue<glm::mat3>(type_id_, location_, value_) ||
      SendActualValue<glm::mat4>(type_id_, location_, value_) ||
      SendActualValue<SamplerUniform>(type_id_, location_, value_);

  [[unlikely]] if (!type_found) {
    reflection::TypeHandle type_handle{type_id_};
    throw std::runtime_error(fmt::format("Invalid type {}", type_handle->name));
  }
}

void ShaderUniform::SetType(ui32 type_id) {
  Clear();
  auto type_info = reflection::GetTypeInfo(type_id);
  [[unlikely]] if (!type_info) { throw std::runtime_error("Unknown type id"); }

  type_id_ = type_id;
  value_.resize(type_info->size);
  type_info->default_constructor(value_.data());
}

void ShaderUniform::EnsureTypeMatch(ui32 type_id) const {
  [[unlikely]] if (GetType() != type_id) {
    throw std::runtime_error(
        "Trying to assign a value of invalid type to uniform");
  }
}

bool ShaderUniform::IsEmpty() const noexcept { return value_.empty(); }

void ShaderUniform::SetValue(std::span<const ui8> value) {
  CheckNotEmpty();
  sent_ = false;
  reflection::TypeHandle type_handle{type_id_};
  assert(type_handle->size == value.size());
  type_handle->copy_assign(value_.data(), value.data());
}

ShaderUniform& ShaderUniform::operator=(ShaderUniform&& another) {
  MoveFrom(another);
  return *this;
}

void ShaderUniform::Clear() {
  if (!IsEmpty()) {
    reflection::TypeHandle type_handle{type_id_};
    type_handle->destructor(value_.data());
    value_.clear();
    sent_ = false;
  }
}

void ShaderUniform::CheckNotEmpty() const {
  [[unlikely]] if (IsEmpty()) {
    throw std::logic_error("Trying to use empty uniform");
  }
}
