#pragma once

#include "components/component.hpp"
#include "reflection/glm_reflect.hpp"

class TransformComponent : public SimpleComponentBase<TransformComponent> {
 public:
  TransformComponent() = default;
  ~TransformComponent() = default;

  glm::mat4 transform = glm::mat4(1.0f);
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<TransformComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<TransformComponent>(
               "TransformComponent",
               edt::GUID::Create("2B10B91A-661A-413D-978C-3B9BCD9BB5D0"))
        .Base<Component>()
        .Field<"transform", &TransformComponent::transform>();
  }
};
}  // namespace cppreflection