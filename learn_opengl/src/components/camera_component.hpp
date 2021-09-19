#pragma once

#include "components/component.hpp"
#include "wrap/wrap_glm.hpp"

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

namespace reflection {
template <>
struct TypeReflector<CameraComponent> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection