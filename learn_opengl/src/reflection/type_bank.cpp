#include "reflection/type_bank.hpp"

#include "reflection/reflection.hpp"

namespace reflection {

TypeBank::TypeBank() = default;

TypeBank::~TypeBank() = default;

[[nodiscard]] TypeInfo& TypeBank::AllocTypeInfo() noexcept {
  const ui32 id = static_cast<ui32>(types_.size());
  types_.emplace_back();
  TypeInfo& type_info = types_.back();
  type_info.id = id;
  return type_info;
}

const TypeInfo* TypeBank::FindTypeInfo(ui32 id) const noexcept {
  if (id < types_.size()) {
    return &types_[id];
  }

  return nullptr;
}

TypeInfo* TypeBank::FindTypeInfo(ui32 id) noexcept {
  if (id < types_.size()) {
    return &types_[id];
  }

  return nullptr;
}

}  // namespace reflection