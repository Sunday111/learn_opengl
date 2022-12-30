#pragma once

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "EverydayTools/GUID.hpp"
#include "integer.hpp"

class Component {
 public:
  [[nodiscard]] virtual edt::GUID GetTypeGUID() const noexcept = 0;
  virtual void DrawDetails();
  virtual ~Component() noexcept = default;
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<Component> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return cppreflection::StaticClassTypeInfo<Component>(
        "Component", edt::GUID::Create("908C7A01-7B36-4D05-B632-BE05C1ABF57E"));
  }
};
}  // namespace cppreflection

template <typename T>
class SimpleComponentBase : public Component {
 public:
  [[nodiscard]] virtual edt::GUID GetTypeGUID() const noexcept override {
    return cppreflection::GetStaticTypeInfo<T>().guid;
  }
};
