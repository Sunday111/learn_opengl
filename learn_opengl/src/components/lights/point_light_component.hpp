#pragma once

#include "components/component.hpp"
#include "components/lights/attenuation.hpp"
#include "reflection/glm_reflect.hpp"

class PointLightComponent : public SimpleComponentBase<PointLightComponent> {
 public:
  PointLightComponent() = default;
  ~PointLightComponent() = default;

  glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
  Attenuation attenuation;
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<PointLightComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<PointLightComponent>(
               "PointLightComponent",
               edt::GUID::Create("3E1C9A2F-075C-4673-BBE5-0787A68857C0"))
        .Base<Component>()
        .Field<"ambient", &PointLightComponent::ambient>()
        .Field<"diffuse", &PointLightComponent::diffuse>()
        .Field<"specular", &PointLightComponent::specular>()
        .Field<"attenuation", &PointLightComponent::attenuation>();
  }
};
}  // namespace cppreflection
