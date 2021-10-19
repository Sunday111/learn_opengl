#include "components/lights/directional_light_component.hpp"

#include "reflection/glm_reflect.hpp"

DirectionalLightComponent::DirectionalLightComponent() = default;
DirectionalLightComponent::~DirectionalLightComponent() = default;

namespace reflection {
void TypeReflector<DirectionalLightComponent>::ReflectType(TypeHandle handle) {
  handle->name = "PointLightComponent";
  handle->guid = "1FCC9CE4-651B-442E-B4D3-210CB5248975";
  handle.Add<&DirectionalLightComponent::ambient>("ambient");
  handle.Add<&DirectionalLightComponent::diffuse>("diffuse");
  handle.Add<&DirectionalLightComponent::specular>("specular");
  handle.SetBaseClass<Component>();
}
}  // namespace reflection