#pragma once

#include <memory>
#include <vector>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "integer.hpp"

class Entity;

class World {
 public:
  World();
  ~World();

  template <typename T>
  T& SpawnEntity();
  void ForEachEntity(auto&& fn);
  Entity& SpawnEntity(edt::GUID type_id);

  [[nodiscard]] inline size_t GetNumEntities() const noexcept {
    return entities_.size();
  }

  [[nodiscard]] Entity* GetEntityByIndex(size_t index) const noexcept {
    [[likely]] if (index < entities_.size()) { return entities_[index].get(); }
    return nullptr;
  }

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
  Entity& entity_base = SpawnEntity(cppreflection::GetStaticTypeInfo<T>().guid);
  return static_cast<T&>(entity_base);
}
