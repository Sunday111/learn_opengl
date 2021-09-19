#pragma once

#include "reflection/reflection.hpp"
#include "wrap/wrap_glm.hpp"

namespace reflection {
template <>
struct TypeReflector<glm::vec4> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "glm::vec4";
    handle->guid = "1D2F4DA1-5416-4087-8543-820902BBACB2";
  }
};
template <>
struct TypeReflector<glm::vec3> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "glm::vec3";
    handle->guid = "7AEE11B0-DCCB-4AFC-AD00-5B8EA4A0E015";
  }
};
template <>
struct TypeReflector<glm::vec2> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "glm::vec2";
    handle->guid = "B01CA829-0E80-4CD9-95E9-D7F32266F093";
  }
};
template <>
struct TypeReflector<glm::mat4> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "glm::mat4";
    handle->guid = "9B24C2C7-29CD-45F6-AC74-880866D492D0";
  }
};
}  // namespace reflection