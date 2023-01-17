#pragma once

#include "CppReflection/GetTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "EverydayTools/GUID_fmtlib.hpp"
#include "components/camera_component.hpp"
#include "components/lights/directional_light_component.hpp"
#include "components/lights/point_light_component.hpp"
#include "components/lights/spot_light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/transform_component.hpp"
#include "entities/entity.hpp"
#include "reflection/eigen_reflect.hpp"
#include "reflection/glm_reflect.hpp"
#include "shader/sampler_uniform.hpp"
#include "spdlog/spdlog.h"

inline void RegisterReflectionTypes() {
  [[maybe_unused]] const cppreflection::Type* t;
  t = cppreflection::GetTypeInfo<int8_t>();
  t = cppreflection::GetTypeInfo<int16_t>();
  t = cppreflection::GetTypeInfo<int32_t>();
  t = cppreflection::GetTypeInfo<int64_t>();
  t = cppreflection::GetTypeInfo<uint8_t>();
  t = cppreflection::GetTypeInfo<uint16_t>();
  t = cppreflection::GetTypeInfo<uint32_t>();
  t = cppreflection::GetTypeInfo<uint64_t>();
  t = cppreflection::GetTypeInfo<glm::vec2>();
  t = cppreflection::GetTypeInfo<Eigen::Vector3f>();
  t = cppreflection::GetTypeInfo<Eigen::Vector4f>();
  t = cppreflection::GetTypeInfo<Eigen::Matrix3f>();
  t = cppreflection::GetTypeInfo<Eigen::Matrix4f>();
  t = cppreflection::GetTypeInfo<Eigen::Vector2f>();
  t = cppreflection::GetTypeInfo<Eigen::Vector3f>();
  t = cppreflection::GetTypeInfo<Eigen::Vector4f>();
  t = cppreflection::GetTypeInfo<Eigen::Matrix3f>();
  t = cppreflection::GetTypeInfo<Eigen::Matrix4f>();
  t = cppreflection::GetTypeInfo<Component>();
  t = cppreflection::GetTypeInfo<CameraComponent>();
  t = cppreflection::GetTypeInfo<MeshComponent>();
  t = cppreflection::GetTypeInfo<TransformComponent>();
  t = cppreflection::GetTypeInfo<DirectionalLightComponent>();
  t = cppreflection::GetTypeInfo<PointLightComponent>();
  t = cppreflection::GetTypeInfo<SpotLightComponent>();
  t = cppreflection::GetTypeInfo<Attenuation>();
  t = cppreflection::GetTypeInfo<Entity>();
  t = cppreflection::GetTypeInfo<SamplerUniform>();
}