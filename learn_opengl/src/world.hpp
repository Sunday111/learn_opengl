#pragma once

#include <memory>
#include <vector>

#include "integer.hpp"
#include "reflection/reflection.hpp"

class Entity;

class EntityDeleter {
 public:
  void operator()(Entity*) const;
};

class World {
 public:
  World();
  ~World();

  void ForEachEntity(auto&& fn) {
    for (auto& entity : entities_) {
      fn(*entity);
    }
  }

  Entity& SpawnEntity(ui32 type_id);

  template <typename T>
  T& SpawnEntity() {
    const ui32 type_id = reflection::GetTypeId<T>();
    Entity& entity_base = SpawnEntity(type_id);
    return static_cast<T&>(entity_base);
  }

 private:
  using EntityPtr = std::unique_ptr<Entity, EntityDeleter>;

 private:
  std::vector<EntityPtr> entities_;
};