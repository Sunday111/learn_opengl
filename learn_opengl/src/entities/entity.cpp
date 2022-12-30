#include "entities/entity.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "CppReflection/TypeRegistry.hpp"
#include "EverydayTools/GUID_fmtlib.hpp"
#include "components/component.hpp"
#include "imgui.h"
#include "memory/memory.hpp"

void Entity::ComponentDeleter::operator()(Component* component) const {
  component->~Component();
  Memory::AlignedFree(component);
}

Entity::Entity() = default;
Entity::~Entity() noexcept = default;
edt::GUID Entity::GetTypeGUID() const noexcept {
  return cppreflection::GetStaticTypeInfo<Entity>().guid;
}

Component* Entity::AddComponent(edt::GUID type_guid) {
  cppreflection::TypeRegistry* type_registry = cppreflection::GetTypeRegistry();
  const cppreflection::Type* type_info = type_registry->FindType(type_guid);

  if (!type_info) {
    throw std::runtime_error(fmt::format("Unknown type: {}", type_guid));
  }

  constexpr edt::GUID component_guid =
      cppreflection::GetStaticTypeInfo<Component>().guid;

  [[unlikely]] if (!type_info->IsA(component_guid)) {
    throw std::runtime_error(
        fmt::format("{} is not a component", type_info->GetName()));
  }
  void* memory = Memory::AlignedAlloc(type_info->GetInstanceSize(),
                                      type_info->GetAlignment());
  type_info->GetSpecialMembers().defaultConstructor(memory);
  ComponentPtr component(reinterpret_cast<Component*>(memory));
  components_.push_back(std::move(component));
  return components_.back().get();
}

void Entity::DrawDetails() {
  if (ImGui::TreeNode(name_.data())) {
    if (ImGui::TreeNode("Components")) {
      ForEachComp([&](Component& c) { c.DrawDetails(); });
      ImGui::TreePop();
    }
    ImGui::TreePop();
  }
}

void Entity::SetName(const std::string_view& name) { name_ = name; }