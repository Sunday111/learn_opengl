#pragma once

#include "CppReflection/GetStaticTypeInfo.hpp"

struct Attenuation {
  float constant = 1.0f;
  float linear = 0.09f;
  float quadratic = 0.032f;
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<Attenuation> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<Attenuation>(
               "Attenuation",
               edt::GUID::Create("FB0EDDB4-7D52-41B3-89EB-7294C25ECEEB"))
        .Field<"constant", &Attenuation::constant>()
        .Field<"linear", &Attenuation::linear>()
        .Field<"quadratic", &Attenuation::quadratic>();
  }
};
}  // namespace cppreflection