#include "components/transform_component.hpp"

#include "reflection/glm_reflect.hpp"

TransformComponent::TransformComponent() = default;
TransformComponent::~TransformComponent() = default;

namespace reflection {
void TypeReflector<TransformComponent>::ReflectType(TypeHandle handle) {
  handle->name = "TransformComponent";
  handle->guid = "2B10B91A-661A-413D-978C-3B9BCD9BB5D0";
  handle.Add<&TransformComponent::transform>("transform");
  handle.SetBaseClass<Component>();
}
}  // namespace reflection