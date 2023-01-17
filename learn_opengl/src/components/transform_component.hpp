#pragma once

#include "components/component.hpp"
#include "reflection/eigen_reflect.hpp"

class TransformComponent : public SimpleComponentBase<TransformComponent> {
 public:
  TransformComponent() {
    Eigen::Transform<float, 3, Eigen::Affine> tr;
    tr.matrix().setIdentity();
    transform = tr.matrix();
  }
  ~TransformComponent() = default;

  [[nodiscard]] Eigen::Vector3f GetTranslation() const {
    Eigen::Transform<float, 3, Eigen::Affine> tr(transform);
    return tr.translation();
  }

  [[nodiscard]] Eigen::Matrix3f GetRotationMtx() const {
    return transform.block<3, 3>(0, 0);
  }

  Eigen::Matrix4f transform;
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<TransformComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<TransformComponent>(
               "TransformComponent",
               edt::GUID::Create("2B10B91A-661A-413D-978C-3B9BCD9BB5D0"))
        .Base<Component>()
        .Field<"transform", &TransformComponent::transform>();
  }
};
}  // namespace cppreflection