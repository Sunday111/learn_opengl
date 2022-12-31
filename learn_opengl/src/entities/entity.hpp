#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "EverydayTools/GUID.hpp"
#include "components/component.hpp"
#include "integer.hpp"

class Entity {
 public:
  Entity();
  Entity(const Entity&) = delete;
  virtual ~Entity() noexcept;

  virtual void DrawDetails();
  void SetName(const std::string_view& name);
  void SetId(size_t id) { id_ = id; }
  [[nodiscard]] size_t GetId() const noexcept { return id_; }
  [[nodiscard]] const std::string_view GetName() const { return name_; }

  Entity& operator=(const Entity&) = delete;

  template <typename T = Component, typename F>
  void ForEachComp(F&& f);

  template <typename T>
  T& AddComponent();
  [[nodiscard]] virtual edt::GUID GetTypeGUID() const noexcept;
  Component* AddComponent(edt::GUID type_guid);

 private:
  class ComponentDeleter {
   public:
    void operator()(Component*) const;
  };
  using ComponentPtr = std::unique_ptr<Component, ComponentDeleter>;

 private:
  std::string name_;
  std::vector<ComponentPtr> components_;
  size_t id_;
};

template <typename T, typename F>
void Entity::ForEachComp(F&& f) {
  cppreflection::TypeRegistry* type_registry = cppreflection::GetTypeRegistry();
  constexpr edt::GUID filter_guid = cppreflection::GetStaticTypeGUID<T>();
  for (auto& component : components_) {
    const auto type_guid = component->GetTypeGUID();
    const cppreflection::Type* type = type_registry->FindType(type_guid);
    if (type->IsA(filter_guid)) {
      f(*static_cast<T*>(component.get()));
    }
  }
}

template <typename T>
T& Entity::AddComponent() {
  const auto type_guid = cppreflection::GetStaticTypeInfo<T>().guid;
  Component* component_base = AddComponent(type_guid);
  return *static_cast<T*>(component_base);
}

template <typename T>
class SimpleEntityBase : public Entity {
 public:
  [[nodiscard]] virtual edt::GUID GetTypeGUID() const noexcept override {
    return cppreflection::GetStaticTypeInfo<T>().guid;
  }
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<Entity> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Entity>(
        "Entity", edt::GUID::Create("E5CACCEE-51D1-4180-AADB-00AD77469579"));
  }
};
}  // namespace cppreflection