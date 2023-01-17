#include "components/camera_component.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

#include "EverydayTools/Math/deg_to_rad.hpp"
#include "reflection/eigen_reflect.hpp"

[[nodiscard]] Eigen::Matrix4f CameraComponent::GetProjection(
    float aspect) const noexcept {
  return Eigen::Perspective(edt::DegToRad(fov), aspect, near_plane, far_plane);
}

[[nodiscard]] Eigen::Matrix4f CameraComponent::GetView() const noexcept {
  return Eigen::LookAt<Eigen::Vector3f>(eye, eye + front, up);
}

void CameraComponent::AddInput(Eigen::Vector3f YawPitchRoll) {
  r.array() += YawPitchRoll.array() * Eigen::Array3f(1.0f, -1.f, 1.f);
  r.y() = std::clamp(r.y(), edt::DegToRad(-89.0f), edt::DegToRad(89.0f));

  front.x() = std::sin(r.x()) * std::cos(r.y());
  front.y() = std::cos(r.x()) * std::cos(r.y());
  front.z() = std::sin(r.y());
  front = front.normalized();
}
