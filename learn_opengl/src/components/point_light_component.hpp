#pragma once

#include "components/component.hpp"
#include "wrap/wrap_glm.hpp"

class PointLightComponent : public SimpleComponentBase<PointLightComponent> {
 public:
  PointLightComponent();
  ~PointLightComponent();

  glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
};

namespace reflection {
template <>
struct TypeReflector<PointLightComponent> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection