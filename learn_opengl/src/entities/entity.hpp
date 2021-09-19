#pragma once

#include <memory>
#include <vector>

#include "integer.hpp"
#include "reflection/reflection.hpp"

class Component;

class ComponentDeleter {
 public:
  void operator()(Component*) const;
};

class Entity {
 public:
  Entity();
  Entity(const Entity&) = delete;
  virtual ~Entity() noexcept;

  Entity& operator=(const Entity&) = delete;

  [[nodiscard]] virtual ui32 GetTypeId() const noexcept;

  Component* AddComponent(ui32 type_id);

  template <typename T, typename F>
  void ForEachComp(F&& f) {
    for (auto& component : components_) {
      const ui32 type_id = component->GetTypeId();
      const reflection::TypeHandle type_info{type_id};
      if (type_info.IsA<T>()) {
        f(*static_cast<T*>(component.get()));
      }
    }
  }

  template <typename T>
  T& AddComponent() {
    const ui32 type_id = reflection::GetTypeId<T>();
    Component* component_base = AddComponent(type_id);
    return *static_cast<T*>(component_base);
  }

 private:
  using ComponentPtr = std::unique_ptr<Component, ComponentDeleter>;

 private:
  std::vector<ComponentPtr> components_;
};

template <typename T>
class SimpleEntityBase : public Entity {
 public:
  [[nodiscard]] virtual ui32 GetTypeId() const noexcept override {
    return reflection::GetTypeId<T>();
  }
};

namespace reflection {
template <>
struct TypeReflector<Entity> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "Entity";
    handle->guid = "E5CACCEE-51D1-4180-AADB-00AD77469579";
  }
};
}  // namespace reflection