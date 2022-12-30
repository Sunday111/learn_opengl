#include "world.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "components/component.hpp"
#include "entities/entity.hpp"
#include "memory/memory.hpp"

void World::EntityDeleter::operator()(Entity* entity) const {
  entity->~Entity();
  Memory::AlignedFree(entity);
}

World::World() = default;
World::~World() = default;

Entity& World::SpawnEntity(edt::GUID guid) {
  constexpr edt::GUID entity_type_guid =
      cppreflection::GetStaticTypeInfo<Entity>().guid;
  const cppreflection::Type* type_info =
      cppreflection::GetTypeRegistry()->FindType(guid);
  [[unlikely]] if (!type_info->IsA(entity_type_guid)) {
    throw std::runtime_error(
        fmt::format("{} is not an entity", type_info->GetName()));
  }
  void* memory = Memory::AlignedAlloc(type_info->GetInstanceSize(),
                                      type_info->GetAlignment());
  type_info->GetSpecialMembers().defaultConstructor(memory);
  EntityPtr entity(reinterpret_cast<Entity*>(memory));
  entity->SetId(next_entity_id_++);
  entity->SetName(fmt::format("Entity {}", entity->GetId()));
  entities_.push_back(std::move(entity));
  return *entities_.back();
}
