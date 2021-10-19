#pragma once

#include "reflection/reflection.hpp"

struct Attenuation {
  float constant = 1.0f;
  float linear = 0.09f;
  float quadratic = 0.032f;
};

namespace reflection {
template <>
struct TypeReflector<Attenuation> {
  static void ReflectType(TypeHandle handle);
};
}  // namespace reflection