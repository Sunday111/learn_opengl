#include "components/point_light_component.hpp"

#include "reflection/glm_reflect.hpp"

PointLightComponent::PointLightComponent() = default;
PointLightComponent::~PointLightComponent() = default;

namespace reflection {
void TypeReflector<PointLightComponent>::ReflectType(TypeHandle handle) {
  handle->name = "PointLightComponent";
  handle->guid = "3E1C9A2F-075C-4673-BBE5-0787A68857C0";
  handle.Add<&PointLightComponent::color>("color");
  handle.SetBaseClass<Component>();
}
}  // namespace reflection