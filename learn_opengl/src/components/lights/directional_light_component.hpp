#pragma once

#include "components/component.hpp"
#include "reflection/eigen_reflect.hpp"
#include "wrap/wrap_eigen.hpp"

class DirectionalLightComponent
    : public SimpleComponentBase<DirectionalLightComponent> {
 public:
  DirectionalLightComponent() = default;
  ~DirectionalLightComponent() = default;

  Eigen::Vector3f ambient = Eigen::Vector3f(0.1f, 0.1f, 0.1f);
  Eigen::Vector3f diffuse = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
  Eigen::Vector3f specular = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<DirectionalLightComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<DirectionalLightComponent>(
               "DirectionalLightComponent",
               edt::GUID::Create("1FCC9CE4-651B-442E-B4D3-210CB5248975"))
        .Base<Component>()
        .Field<"ambient", &DirectionalLightComponent::ambient>()
        .Field<"diffuse", &DirectionalLightComponent::diffuse>()
        .Field<"specular", &DirectionalLightComponent::specular>();
  }
};
}  // namespace cppreflection
