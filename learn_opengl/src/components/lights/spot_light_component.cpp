#include "components/lights/spot_light_component.hpp"

#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"

SpotLightComponent::SpotLightComponent() = default;
SpotLightComponent::~SpotLightComponent() = default;

namespace reflection {
void TypeReflector<SpotLightComponent>::ReflectType(TypeHandle handle) {
  handle->name = "SpotLightComponent";
  handle->guid = "D8AEECBB-E69C-45B5-BDFC-F8A725DABF40";
  handle.Add<&SpotLightComponent::attenuation>("attenuation");
  handle.Add<&SpotLightComponent::diffuse>("diffuse");
  handle.Add<&SpotLightComponent::specular>("specular");
  handle.Add<&SpotLightComponent::direction>("direction");
  handle.Add<&SpotLightComponent::innerAngle>("innerAngle");
  handle.Add<&SpotLightComponent::outerAngle>("outerAngle");
  handle.SetBaseClass<Component>();
}
}  // namespace reflection