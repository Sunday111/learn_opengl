#pragma once

#include <span>
#include <string>
#include <vector>

#include "integer.hpp"
#include "name_cache/name.hpp"

class ShaderUniform {
 public:
  ShaderUniform();
  ~ShaderUniform();
  ShaderUniform(const ShaderUniform&) = delete;
  ShaderUniform(ShaderUniform&& another);
  void MoveFrom(ShaderUniform& another);
  void SendValue() const;
  void SetValue(std::span<const ui8> new_value);
  void SetType(ui32 type_id);
  void SetName(Name name) { name_ = name; }
  void SetLocation(ui32 location) { location_ = location; }
  void EnsureTypeMatch(ui32 type_id) const;

  [[nodiscard]] bool IsEmpty() const noexcept;
  [[nodiscard]] Name GetName() const noexcept { return name_; }
  [[nodiscard]] ui32 GetType() const noexcept { return type_id_; }
  [[nodiscard]] ui32 GetLocation() const noexcept { return location_; }

  //[[nodiscard]] std::span<ui8> GetValue() noexcept { return value_; }
  [[nodiscard]] std::span<const ui8> GetValue() const noexcept {
    return value_;
  }

  ShaderUniform& operator=(const ShaderUniform&) = delete;
  ShaderUniform& operator=(ShaderUniform&& another);

 private:
  void Clear();
  void CheckNotEmpty() const;

 private:
  std::vector<ui8> value_;
  Name name_;
  ui32 location_;
  ui32 type_id_;
  mutable bool sent_ = false;
};
