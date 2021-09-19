#pragma once

#include <memory>
#include <vector>

#include "integer.hpp"
#include "reflection/reflection.hpp"

class Entity;

class World {
 public:
  World();
  ~World();

  template <typename T>
  T& SpawnEntity();
  void ForEachEntity(auto&& fn);
  Entity& SpawnEntity(ui32 type_id);

 private:
  class EntityDeleter {
   public:
    void operator()(Entity*) const;
  };

  using EntityPtr = std::unique_ptr<Entity, EntityDeleter>;

 private:
  std::vector<EntityPtr> entities_;
  size_t next_entity_id_ = 0;
};

void World::ForEachEntity(auto&& fn) {
  for (auto& entity : entities_) {
    fn(*entity);
  }
}

template <typename T>
T& World::SpawnEntity() {
  const ui32 type_id = reflection::GetTypeId<T>();
  Entity& entity_base = SpawnEntity(type_id);
  return static_cast<T&>(entity_base);
}
