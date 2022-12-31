#include "components/type_id_widget.hpp"

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "integer.hpp"
#include "reflection/glm_reflect.hpp"
#include "wrap/wrap_glm.hpp"
#include "wrap/wrap_imgui.h"

template <typename T>
bool ScalarProperty(edt::GUID type_guid, std::string_view name, void* address,
                    bool& value_changed,
                    T min = std::numeric_limits<T>::lowest(),
                    T max = std::numeric_limits<T>::max()) {
  constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
  if (type_info.guid == type_guid) {
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
bool VectorProperty(edt::GUID type_guid, std::string_view name, void* address,
                    bool& value_changed) {
  constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
  if (type_info.guid == type_guid) {
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
bool MatrixProperty(edt::GUID type_guid, std::string_view name, void* address,
                    bool& value_changed) {
  constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
  if (type_info.guid == type_guid) {
    T& member_ref = *reinterpret_cast<T*>(address);
    value_changed |= MatrixProperty(name, member_ref);
    return true;
  }

  return false;
}

void SimpleTypeWidget(edt::GUID type_guid, std::string_view name, void* value,
                      bool& value_changed) {
  value_changed = false;
  [[maybe_unused]] const bool found_type =
      ScalarProperty<float>(type_guid, name, value, value_changed) ||
      ScalarProperty<double>(type_guid, name, value, value_changed) ||
      ScalarProperty<ui8>(type_guid, name, value, value_changed) ||
      ScalarProperty<ui16>(type_guid, name, value, value_changed) ||
      ScalarProperty<ui32>(type_guid, name, value, value_changed) ||
      ScalarProperty<ui64>(type_guid, name, value, value_changed) ||
      ScalarProperty<i8>(type_guid, name, value, value_changed) ||
      ScalarProperty<i16>(type_guid, name, value, value_changed) ||
      ScalarProperty<i32>(type_guid, name, value, value_changed) ||
      ScalarProperty<i64>(type_guid, name, value, value_changed) ||
      VectorProperty<glm::vec2>(type_guid, name, value, value_changed) ||
      VectorProperty<glm::vec3>(type_guid, name, value, value_changed) ||
      VectorProperty<glm::vec4>(type_guid, name, value, value_changed) ||
      MatrixProperty<glm::mat4>(type_guid, name, value, value_changed);
}

void TypeIdWidget(edt::GUID type_guid, void* base, bool& value_changed) {
  const cppreflection::Type* type_info =
      cppreflection::GetTypeRegistry()->FindType(type_guid);
  for (const cppreflection::Field* field : type_info->GetFields()) {
    void* pmember = field->GetValue(base);
    bool member_changed = false;
    SimpleTypeWidget(field->GetType()->GetGuid(), field->GetName(), pmember,
                     member_changed);
    value_changed |= member_changed;
  }
}
