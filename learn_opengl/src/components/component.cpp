#include "components/component.hpp"

#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "wrap/wrap_glm.hpp"
#include "wrap/wrap_imgui.h"

template <typename T>
bool ScalarProperty(const reflection::TypeVariable& variable, void* base,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) {
  if (variable.type_id == reflection::GetTypeId<T>()) {
    const char* format = "%.3f";
    if constexpr (std::is_floating_point_v<T>) {
      format = "%d";
    }

    T* value = variable.GetPtr<T>(base);
    ImGui::DragScalar(variable.name.data(), CastDataType<T>(), value, 1.0f,
                      &min, &max);
    return true;
  }

  return false;
}

template <typename T, int N>
bool VectorProperty(const char* title, glm::vec<N, T>& value,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) noexcept {
  return ImGui::DragScalarN(title, CastDataType<T>(), glm::value_ptr(value), N,
                            0.01f, &min, &max, "%.3f");
}

template <typename T>
bool VectorProperty(const reflection::TypeVariable& variable, void* base) {
  if (variable.type_id == reflection::GetTypeId<T>()) {
    const ui64 member_address = reinterpret_cast<ui64>(base) + variable.offset;
    T& member_ref = *reinterpret_cast<T*>(member_address);
    VectorProperty(variable.name.data(), member_ref);
    return true;
  }

  return false;
}

void Component::DrawDetails() {
  reflection::TypeHandle type_info{GetTypeId()};
  for (auto& variable : type_info->variables) {
    [[maybe_unused]] const bool found_type =
        ScalarProperty<float>(variable, this) ||
        ScalarProperty<double>(variable, this) ||
        ScalarProperty<ui8>(variable, this) ||
        ScalarProperty<ui16>(variable, this) ||
        ScalarProperty<ui32>(variable, this) ||
        ScalarProperty<ui64>(variable, this) ||
        ScalarProperty<i8>(variable, this) ||
        ScalarProperty<i16>(variable, this) ||
        ScalarProperty<i32>(variable, this) ||
        ScalarProperty<i64>(variable, this) ||
        VectorProperty<glm::vec2>(variable, this) ||
        VectorProperty<glm::vec3>(variable, this) ||
        VectorProperty<glm::vec4>(variable, this);
  }
}