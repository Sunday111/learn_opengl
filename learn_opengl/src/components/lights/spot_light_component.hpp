#pragma once

#include "components/component.hpp"
#include "components/lights/attenuation.hpp"
#include "wrap/wrap_glm.hpp"

class SpotLightComponent : public SimpleComponentBase<SpotLightComponent> {
 public:
  SpotLightComponent();
  ~SpotLightComponent();

  Attenuation attenuation;
  glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 direction = glm::vec3(1.0f, 0.0f, 0.0f);
  float innerAngle;
  float outerAngle;
};

namespace reflection {
template <>
struct TypeReflector<SpotLightComponent> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection