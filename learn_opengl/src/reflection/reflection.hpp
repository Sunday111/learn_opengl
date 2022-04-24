#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "reflection/type_bank.hpp"
#include "template/class_member_traits.hpp"
#include "template/member_offset.hpp"

namespace reflection {

template <typename T>
struct TypeReflector;

template <typename T>
ui32 GetTypeId();

struct TypeVariable {
  [[nodiscard]] void* GetPtr(void* base) const noexcept;
  [[nodiscard]] const void* GetPtr(const void* base) const noexcept;

  template <typename T>
  [[nodiscard]] T* GetPtr(void* base) const noexcept;

  template <typename T>
  [[nodiscard]] const T* GetPtr(const void* base) const noexcept;

  std::string name;
  ui32 type_id;
  ui16 offset;
};

struct TypeMethod {
  std::string name;
};

struct TypeInfo;

struct ReflectionHelper {
  [[nodiscard]] static TypeInfo* GetTypeInfo(ui32 type_id);
};

template <typename T>
[[nodiscard]] TypeHandle GetTypeInfo();
[[nodiscard]] TypeInfo* GetTypeInfo(ui32 type_id);

class TypeHandle {
 public:
  TypeInfo* operator->() const { return Get(); }

  [[nodiscard]] bool IsA(ui32 base) const noexcept;

  template <typename T>
  [[nodiscard]] bool IsA() const noexcept {
    return IsA(GetTypeId<T>());
  }

  TypeInfo* Get() const { return ReflectionHelper::GetTypeInfo(type_id); }

  template <auto member>
  void Add(std::string_view name) const;

  template <typename Base>
  void SetBaseClass() const;

  ui32 type_id;
};

struct TypeInfo {
  using DefaultConstructor = void (*)(void* value);
  using CopyConstructor = void (*)(void* value, const void* arg);
  using MoveConstructor = void (*)(void* value, void* arg);
  using CopyAssign = void (*)(void* value, const void* arg);
  using MoveAssign = void (*)(void* value, void* arg);
  using Destructor = void (*)(void* value);

  std::string name;
  std::string guid;
  std::vector<TypeVariable> variables;
  std::vector<TypeMethod> methods;
  DefaultConstructor default_constructor = nullptr;
  CopyConstructor copy_constructor = nullptr;
  MoveConstructor move_constructor = nullptr;
  CopyAssign copy_assign = nullptr;
  MoveAssign move_assign = nullptr;
  Destructor destructor = nullptr;
  ui32 id;
  ui32 alignment = 0;
  ui32 size = 0;
  std::optional<ui32> base;

  TypeInfo() = default;
  TypeInfo(const TypeInfo&) = delete;
  TypeInfo(TypeInfo&&) = default;
};

template <auto member>
void TypeHandle::Add(std::string_view name) const {
  using Traits = ClassMemberTraits<decltype(member)>;

  if constexpr (Traits::IsVariable()) {
    using Member = typename Traits::Member;
    static_assert(!std::is_pointer_v<Member>,
                  "pointer members are not supported yet");
    TypeHandle member_type = GetTypeInfo<Member>();
    TypeVariable v;
    v.name = name;
    v.type_id = member_type->id;

    const size_t offset = MemberOffset<member>();
    assert(std::numeric_limits<decltype(v.offset)>::max() >= offset);
    v.offset = static_cast<decltype(v.offset)>(offset);
    Get()->variables.push_back(std::move(v));
  }

  if constexpr (Traits::IsFunction()) {
    TypeMethod m;
    m.name = name;
    Get()->methods.push_back(std::move(m));
  }
}

template <typename Base>
void TypeHandle::SetBaseClass() const {
  const ui32 base_type_id = GetTypeId<Base>();
  Get()->base = base_type_id;
}

template <typename T>
ui32 GetTypeId() {
  auto init = []() -> ui32 {
    auto& type_bank = TypeBank::Instance();
    TypeInfo& ti = type_bank.AllocTypeInfo();
    ti.size = sizeof(T);
    ti.alignment = alignof(T);

    if constexpr (!std::is_abstract_v<T>) {
      if constexpr (std::is_default_constructible_v<T>) {
        ti.default_constructor = [](void* data) { new (data) T(); };
      }

      if constexpr (std::is_move_constructible_v<T>) {
        ti.move_constructor = [](void* to, void* from) {
          new (to) T(std::move(*reinterpret_cast<T*>(from)));
        };
      }

      if constexpr (std::is_copy_constructible_v<T>) {
        ti.copy_constructor = [](void* to, const void* from) {
          new (to) T(*reinterpret_cast<const T*>(from));
        };
      }

      if constexpr (std::is_copy_assignable_v<T>) {
        ti.copy_assign = [](void* value, const void* arg) {
          new (value) T(*reinterpret_cast<const T*>(arg));
        };
      }

      if constexpr (std::is_move_assignable_v<T>) {
        ti.move_assign = [](void* value, void* arg) {
          new (value) T(std::move(*reinterpret_cast<T*>(arg)));
        };
      }
    }

    ui32 id = ti.id;
    ti.destructor = [](void* value) { reinterpret_cast<T*>(value)->~T(); };

    TypeReflector<T>::ReflectType(TypeHandle{id});
    return id;
  };

  static ui32 type_id = init();
  return type_id;
}

template <typename T>
TypeHandle GetTypeInfo() {
  return TypeHandle{GetTypeId<T>()};
}

template <typename T>
T* TypeVariable::GetPtr(void* base) const noexcept {
  assert(GetTypeId<T>() == type_id);
  return reinterpret_cast<T*>(GetPtr(base));
}

template <typename T>
const T* TypeVariable::GetPtr(const void* base) const noexcept {
  assert(GetTypeId<T>() == type_id);
  return reinterpret_cast<const T*>(GetPtr(base));
}

}  // namespace reflection