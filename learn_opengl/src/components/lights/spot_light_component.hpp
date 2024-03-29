#pragma once

#include "components/component.hpp"
#include "components/lights/attenuation.hpp"
#include "reflection/eigen_reflect.hpp"

class SpotLightComponent : public SimpleComponentBase<SpotLightComponent> {
 public:
  SpotLightComponent() = default;
  ~SpotLightComponent() = default;

  Attenuation attenuation;
  Eigen::Vector3f diffuse = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
  Eigen::Vector3f specular = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
  Eigen::Vector3f direction = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
  float innerAngle;
  float outerAngle;
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<SpotLightComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<SpotLightComponent>(
               "SpotLightComponent",
               edt::GUID::Create("3E1C9A2F-075C-4673-BBE5-0787A68857C0"))
        .Base<Component>()
        .Field<"attenuation", &SpotLightComponent::attenuation>()
        .Field<"diffuse", &SpotLightComponent::diffuse>()
        .Field<"specular", &SpotLightComponent::specular>()
        .Field<"direction", &SpotLightComponent::direction>()
        .Field<"innerAngle", &SpotLightComponent::innerAngle>()
        .Field<"outerAngle", &SpotLightComponent::outerAngle>();
  }
};
}  // namespace cppreflection