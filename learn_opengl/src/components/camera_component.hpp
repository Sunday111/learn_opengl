#pragma once

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "components/component.hpp"
#include "reflection/glm_reflect.hpp"

class CameraComponent : public SimpleComponentBase<CameraComponent> {
 public:
  [[nodiscard]] glm::mat4 GetProjection(float aspect) const noexcept;
  [[nodiscard]] glm::mat4 GetView() const noexcept;
  void AddInput(glm::vec3 YawPitchRoll);

  float speed = 1.0f;
  float near_plane = 0.01f;
  float far_plane = 1000.0f;
  float fov = 45.0f;
  glm::vec3 eye = {0.0f, 0.0f, 3.0f};
  glm::vec3 front = {1.0f, 0.0f, 0.0f};
  glm::vec3 up = {0.0f, 0.0f, 1.0f};

 private:
  glm::vec3 r = {0.0f, 0.0f, 0.0f};
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<CameraComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<CameraComponent>(
               "CameraComponent",
               edt::GUID::Create("8E4717C2-65B2-41C8-AAA6-91285A671314"))
        .Base<Component>()
        .Field<"speed", &CameraComponent::speed>()
        .Field<"near_plane", &CameraComponent::near_plane>()
        .Field<"far_plane", &CameraComponent::far_plane>()
        .Field<"eye", &CameraComponent::eye>()
        .Field<"front", &CameraComponent::front>()
        .Field<"up", &CameraComponent::up>();
  }
};
}  // namespace cppreflection