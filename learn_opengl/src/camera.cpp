#include "camera.hpp"

#include <algorithm>

[[nodiscard]] glm::mat4 Camera::GetProjection(float aspect) const noexcept {
  return glm::perspective(glm::radians(fov), aspect, near_plane, far_plane);
}

[[nodiscard]] glm::mat4 Camera::GetView() const noexcept {
  return glm::lookAt(eye, eye + front, up);
}

void Camera::AddInput(glm::vec3 YawPitchRoll) {
  r += YawPitchRoll * glm::vec3(1.0f, -1.0f, 1.0f);
  r.y = std::clamp(r.y, glm::radians(-89.0f), glm::radians(89.0f));

  front.x = sin(r.x) * cos(r.y);
  front.y = cos(r.x) * cos(r.y);
  front.z = sin(r.y);
  front = glm::normalize(front);
}
