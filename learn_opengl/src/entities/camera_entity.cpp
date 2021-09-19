#include "entities/camera_entity.hpp"

#include <algorithm>

#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"

[[nodiscard]] glm::mat4 CameraEntity::GetProjection(
    float aspect) const noexcept {
  return glm::perspective(glm::radians(fov), aspect, near_plane, far_plane);
}

[[nodiscard]] glm::mat4 CameraEntity::GetView() const noexcept {
  return glm::lookAt(eye, eye + front, up);
}

void CameraEntity::AddInput(glm::vec3 YawPitchRoll) {
  r += YawPitchRoll * glm::vec3(1.0f, -1.0f, 1.0f);
  r.y = std::clamp(r.y, glm::radians(-89.0f), glm::radians(89.0f));

  front.x = sin(r.x) * cos(r.y);
  front.y = cos(r.x) * cos(r.y);
  front.z = sin(r.y);
  front = glm::normalize(front);
}

namespace reflection {
void TypeReflector<CameraEntity>::ReflectType(TypeHandle handle) {
  handle->name = "CameraEntity";
  handle->guid = "8E4717C2-65B2-41C8-AAA6-91285A671314";
  handle.Add<&CameraEntity::speed>("speed");
  handle.Add<&CameraEntity::near_plane>("near_plane");
  handle.Add<&CameraEntity::far_plane>("far_plane");
  handle.Add<&CameraEntity::eye>("eye");
  handle.Add<&CameraEntity::front>("front");
  handle.Add<&CameraEntity::up>("up");
}
}  // namespace reflection