#include "reflection/reflection.hpp"

namespace reflection {

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