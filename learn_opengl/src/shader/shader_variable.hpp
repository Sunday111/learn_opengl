#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "integer.hpp"
#include "nlohmann/json.hpp"

class ShaderVariable {
 public:
  ShaderVariable(const ShaderVariable&) = delete;
  ShaderVariable(ShaderVariable&& another);

  ShaderVariable& operator=(const ShaderVariable&) = delete;
  ShaderVariable& operator=(ShaderVariable&& another);

  std::string GenDefine() const;
  void MoveFrom(ShaderVariable& another);
  void SetValue(std::span<const ui8> value_view);

  static ShaderVariable ReadFromJson(const nlohmann::json& json);

 protected:
  ShaderVariable() = default;

 public:
  std::string name;
  std::vector<ui8> value;
  ui32 type_id;
};
