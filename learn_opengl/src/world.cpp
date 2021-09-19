#include "world.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "entities/entity.hpp"
#include "memory/memory.hpp"

void EntityDeleter::operator()(Entity* entity) const {
  entity->~Entity();
  Memory::AlignedFree(entity);
}

World::World() = default;
World::~World() = default;

Entity& World::SpawnEntity(ui32 type_id) {
  const reflection::TypeHandle type_info{type_id};
  [[unlikely]] if (!type_info.IsA<Entity>()) {
    throw std::runtime_error(
        fmt::format("{} is not an entity", type_info->name));
  }
  void* memory = Memory::AlignedAlloc(type_info->size, type_info->alignment);
  type_info->default_constructor(memory);
  EntityPtr entity(reinterpret_cast<Entity*>(memory));
  entities_.push_back(std::move(entity));
  return *entities_.back();
}
