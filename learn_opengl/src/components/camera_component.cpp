#include "components/camera_component.hpp"

#include <algorithm>

#include "reflection/glm_reflect.hpp"

[[nodiscard]] glm::mat4 CameraComponent::GetProjection(
    float aspect) const noexcept {
  return glm::perspective(glm::radians(fov), aspect, near_plane, far_plane);
}

[[nodiscard]] glm::mat4 CameraComponent::GetView() const noexcept {
  return glm::lookAt(eye, eye + front, up);
}

void CameraComponent::AddInput(glm::vec3 YawPitchRoll) {
  r += YawPitchRoll * glm::vec3(1.0f, -1.0f, 1.0f);
  r.y = std::clamp(r.y, glm::radians(-89.0f), glm::radians(89.0f));

  front.x = std::sin(r.x) * std::cos(r.y);
  front.y = std::cos(r.x) * std::cos(r.y);
  front.z = std::sin(r.y);
  front = glm::normalize(front);
}
