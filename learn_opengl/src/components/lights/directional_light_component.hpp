#pragma once

#include "components/component.hpp"
#include "wrap/wrap_glm.hpp"

class DirectionalLightComponent
    : public SimpleComponentBase<DirectionalLightComponent> {
 public:
  DirectionalLightComponent();
  ~DirectionalLightComponent();

  glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
};

namespace reflection {
template <>
struct TypeReflector<DirectionalLightComponent> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection
