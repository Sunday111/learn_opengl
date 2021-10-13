#include "shader/shader_uniform.hpp"

#include <stdexcept>

#include "fmt/format.h"
#include "gl_api.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"

template <typename T>
bool SendActualValue(const ShaderUniform& uniform) {
  if (uniform.type_id == reflection::GetTypeId<T>()) {
    OpenGl::SetUniform(uniform.location,
                       *reinterpret_cast<const T*>(uniform.value.data()));
    return true;
  }

  return false;
}

ShaderUniform::ShaderUniform() = default;

ShaderUniform::ShaderUniform(ShaderUniform&& another) { MoveFrom(another); }

void ShaderUniform::MoveFrom(ShaderUniform& another) {
  name = std::move(another.name);
  value = std::move(another.value);
}

void ShaderUniform::SendValue() const {
  bool type_found =
      SendActualValue<float>(*this) || SendActualValue<glm::vec2>(*this) ||
      SendActualValue<glm::vec3>(*this) || SendActualValue<glm::vec4>(*this) ||
      SendActualValue<glm::mat3>(*this) || SendActualValue<glm::mat4>(*this);

  if (!type_found) {
    reflection::TypeHandle type_handle{type_id};
    throw std::runtime_error(fmt::format("Invalid type {}", type_handle->name));
  }
}

ShaderUniform& ShaderUniform::operator=(ShaderUniform&& another) {
  MoveFrom(another);
  return *this;
}
