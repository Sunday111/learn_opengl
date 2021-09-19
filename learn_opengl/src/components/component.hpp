#pragma once

#include "integer.hpp"
#include "reflection/reflection.hpp"

class Component {
 public:
  [[nodiscard]] virtual ui32 GetTypeId() const noexcept = 0;
  virtual ~Component() noexcept = default;
};

template <typename T>
class SimpleComponentBase : public Component {
 public:
  [[nodiscard]] virtual ui32 GetTypeId() const noexcept override {
    return reflection::GetTypeId<T>();
  }
};