#include "components/type_id_widget.hpp"

#include "integer.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "wrap/wrap_glm.hpp"
#include "wrap/wrap_imgui.h"

template <typename T>
bool ScalarProperty(ui32 type_id, std::string_view name, void* address,
                    bool& value_changed,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) {
  if (type_id == reflection::GetTypeId<T>()) {
    const char* format = "%.3f";
    if constexpr (std::is_floating_point_v<T>) {
      format = "%d";
    }

    T* value = reinterpret_cast<T*>(address);
    const bool c = ImGui::DragScalar(name.data(), CastDataType<T>(), value,
                                     1.0f, &min, &max);
    value_changed = c;
    return true;
  }

  return false;
}

template <typename T, int N>
bool VectorProperty(std::string_view title, glm::vec<N, T>& value,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) noexcept {
  return ImGui::DragScalarN(title.data(), CastDataType<T>(),
                            glm::value_ptr(value), N, 0.01f, &min, &max,
                            "%.3f");
}

template <typename T>
bool VectorProperty(ui32 type_id, std::string_view name, void* address,
                    bool& value_changed) {
  if (type_id == reflection::GetTypeId<T>()) {
    T& member_ref = *reinterpret_cast<T*>(address);
    value_changed |= VectorProperty(name, member_ref);
    return true;
  }

  return false;
}

template <typename T, int C, int R>
bool MatrixProperty(const std::string_view title, glm::mat<C, R, T>& value,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) noexcept {
  bool changed = false;
  if (ImGui::TreeNode(title.data())) {
    for (int row_index = 0; row_index < R; ++row_index) {
      auto row = glm::row(value, row_index);
      ImGui::PushID(row_index);
      const bool row_changed =
          ImGui::DragScalarN("", CastDataType<T>(), glm::value_ptr(row), C,
                             0.01f, &min, &max, "%.3f");
      ImGui::PopID();
      [[unlikely]] if (row_changed) { value = glm::row(value, row_index, row); }
      changed = changed || row_changed;
    }
    ImGui::TreePop();
  }
  return changed;
}

template <typename T>
bool MatrixProperty(ui32 type_id, std::string_view name, void* address,
                    bool& value_changed) {
  if (type_id == reflection::GetTypeId<T>()) {
    T& member_ref = *reinterpret_cast<T*>(address);
    value_changed |= MatrixProperty(name, member_ref);
    return true;
  }

  return false;
}

void SimpleTypeWidget(ui32 type_id, std::string_view name, void* value,
                      bool& value_changed) {
  value_changed = false;
  [[maybe_unused]] const bool found_type =
      ScalarProperty<float>(type_id, name, value, value_changed) ||
      ScalarProperty<double>(type_id, name, value, value_changed) ||
      ScalarProperty<ui8>(type_id, name, value, value_changed) ||
      ScalarProperty<ui16>(type_id, name, value, value_changed) ||
      ScalarProperty<ui32>(type_id, name, value, value_changed) ||
      ScalarProperty<ui64>(type_id, name, value, value_changed) ||
      ScalarProperty<i8>(type_id, name, value, value_changed) ||
      ScalarProperty<i16>(type_id, name, value, value_changed) ||
      ScalarProperty<i32>(type_id, name, value, value_changed) ||
      ScalarProperty<i64>(type_id, name, value, value_changed) ||
      VectorProperty<glm::vec2>(type_id, name, value, value_changed) ||
      VectorProperty<glm::vec3>(type_id, name, value, value_changed) ||
      VectorProperty<glm::vec4>(type_id, name, value, value_changed) ||
      MatrixProperty<glm::mat4>(type_id, name, value, value_changed);
}

void TypeIdWidget(ui32 type_id, void* base, bool& value_changed) {
  reflection::TypeHandle type_info{type_id};
  for (const reflection::TypeVariable& variable : type_info->variables) {
    void* pmember =
        reinterpret_cast<void*>(reinterpret_cast<ui64>(base) + variable.offset);
    bool member_changed = false;
    SimpleTypeWidget(variable.type_id, variable.name, pmember, member_changed);
    value_changed |= member_changed;
  }
}
