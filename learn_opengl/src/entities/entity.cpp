#include "entities/entity.hpp"

#include "components/component.hpp"
#include "memory/memory.hpp"

void ComponentDeleter::operator()(Component* component) const {
  component->~Component();
  Memory::AlignedFree(component);
}

Entity::Entity() = default;
Entity::~Entity() noexcept = default;
ui32 Entity::GetTypeId() const noexcept {
  return reflection::GetTypeId<Entity>();
}

Component* Entity::AddComponent(ui32 type_id) {
  reflection::TypeHandle type_info{type_id};
  void* memory = Memory::AlignedAlloc(type_info->size, type_info->alignment);
  type_info->default_constructor(memory);
  ComponentPtr component(reinterpret_cast<Component*>(memory));
  components_.push_back(std::move(component));
  return components_.back().get();
}