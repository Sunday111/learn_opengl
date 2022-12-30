#pragma once

#include "components/component.hpp"
#include "reflection/glm_reflect.hpp"
#include "wrap/wrap_glm.hpp"

class DirectionalLightComponent
    : public SimpleComponentBase<DirectionalLightComponent> {
 public:
  DirectionalLightComponent() = default;
  ~DirectionalLightComponent() = default;

  glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);
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
