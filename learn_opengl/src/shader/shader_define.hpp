#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "integer.hpp"
#include "nlohmann/json.hpp"

class ShaderDefine {
 public:
  ShaderDefine(const ShaderDefine&) = delete;
  ShaderDefine(ShaderDefine&& another);

  ShaderDefine& operator=(const ShaderDefine&) = delete;
  ShaderDefine& operator=(ShaderDefine&& another);

  std::string GenDefine() const;
  void MoveFrom(ShaderDefine& another);
  void SetValue(std::span<const ui8> value_view);

  static ShaderDefine ReadFromJson(const nlohmann::json& json);

 protected:
  ShaderDefine() = default;

 public:
  std::string name;
  std::vector<ui8> value;
  ui32 type_id;
};
