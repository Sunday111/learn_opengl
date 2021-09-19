#pragma once

#include "integer.hpp"
#include "reflection/reflection.hpp"

class Component {
 public:
  [[nodiscard]] virtual ui32 GetTypeId() const noexcept = 0;
  virtual ~Component() noexcept = default;
};

namespace reflection {
template <>
struct TypeReflector<Component> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "Component";
    handle->guid = "908C7A01-7B36-4D05-B632-BE05C1ABF57E";
  }
};
}  // namespace reflection

template <typename T>
class SimpleComponentBase : public Component {
 public:
  [[nodiscard]] virtual ui32 GetTypeId() const noexcept override {
    return reflection::GetTypeId<T>();
  }
};