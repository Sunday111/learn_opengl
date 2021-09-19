#include "reflection/reflection.hpp"

namespace reflection {

void* TypeVariable::GetPtr(void* base) const noexcept {
  const ui64 member_address = reinterpret_cast<ui64>(base) + offset;
  return reinterpret_cast<void*>(member_address);
}

const void* TypeVariable::GetPtr(const void* base) const noexcept {
  const ui64 member_address = reinterpret_cast<ui64>(base) + offset;
  return reinterpret_cast<void*>(member_address);
}

TypeInfo* ReflectionHelper::GetTypeInfo(ui32 type_id) {
  TypeBank& type_bank = TypeBank::Instance();
  TypeInfo* type_info = type_bank.FindTypeInfo(type_id);
  return type_info;
}

TypeInfo* GetTypeInfo(ui32 type_id) {
  return ReflectionHelper::GetTypeInfo(type_id);
}

bool TypeHandle::IsA(ui32 id) const noexcept {
  std::optional<ui32> cmp_id = type_id;
  do {
    auto ti = ReflectionHelper::GetTypeInfo(*cmp_id);
    if (ti->id == id) {
      return true;
    }

    cmp_id = ti->base;
  } while (cmp_id.has_value());

  return false;
}
}  // namespace reflection