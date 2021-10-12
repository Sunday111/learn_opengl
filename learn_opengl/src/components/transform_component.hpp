#pragma once

#include "components/component.hpp"
#include "wrap/wrap_glm.hpp"

class TransformComponent : public SimpleComponentBase<TransformComponent> {
 public:
  TransformComponent();
  ~TransformComponent();

  glm::mat4 transform = glm::mat4(1.0f);
};

namespace reflection {
template <>
struct TypeReflector<TransformComponent> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection