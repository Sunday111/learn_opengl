#pragma once

#include <memory>

#include "integer.hpp"
#include "reflection/reflection.hpp"
#include "texture/texture.hpp"

class SamplerUniform {
 public:
  std::shared_ptr<Texture> texture;
};

namespace reflection {
template <>
struct TypeReflector<SamplerUniform> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "SamplerUniform";
    handle->guid = "2FBEEB94-BBB3-491C-A299-AD1960641D3F";
  }
};
}  // namespace reflection
