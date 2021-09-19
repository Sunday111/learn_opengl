#pragma once

#include <vector>

#include "integer.hpp"

namespace reflection {
struct TypeInfo;
class TypeHandle;

class TypeBank {
 public:
  static TypeBank& Instance() {
    static TypeBank instance;
    return instance;
  }

  [[nodiscard]] TypeInfo& AllocTypeInfo() noexcept;
  [[nodiscard]] const TypeInfo* FindTypeInfo(ui32 id) const noexcept;

 private:
  friend TypeHandle;
  [[nodiscard]] TypeInfo* FindTypeInfo(ui32 id) noexcept;

 private:
  TypeBank();
  ~TypeBank();

 private:
  std::vector<TypeInfo> types_;
};
}  // namespace reflection