#pragma once

#include <string>
#include <vector>

#include "integer.hpp"
#include "name_cache/name.hpp"

class ShaderUniform {
 public:
  ShaderUniform();
  ShaderUniform(const ShaderUniform&) = delete;
  ShaderUniform(ShaderUniform&& another);
  void MoveFrom(ShaderUniform& another);
  void SendValue() const;
  ShaderUniform& operator=(const ShaderUniform&) = delete;
  ShaderUniform& operator=(ShaderUniform&& another);

  Name name;
  ui32 location;
  ui32 type_id;
  std::vector<ui8> value;
};
