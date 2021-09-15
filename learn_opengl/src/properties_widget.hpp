#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "gl_api.hpp"
#include "include_imgui.h"

template <typename T>
concept Enumeration = std::is_enum_v<T>;

namespace properties_container_impl {
template <typename Index, typename Value>
struct TypedIndex {
  using ValueType = Value;
  using IndexType = Index;
  IndexType index;
};

template <typename T>
struct PropertyTypeStorage {
  std::vector<bool> changed;
  std::vector<T> values;
};

template <typename Index, typename... Types>
struct PropertiesContainer {
 public:
  template <typename T>
  [[nodiscard]] Index AddProperty(T&& initial = T{}) {
    auto& storage = GetTypeStorage<T>();
    const auto full_index = storage.values.size();

    if constexpr (sizeof(full_index) > sizeof(Index)) {
      [[unlikely]] if (full_index > std::numeric_limits<Index>::max()) {
        throw std::runtime_error(
            "Index out of range: too small type for index in properties "
            "container");
      }
    }

    storage.values.push_back(std::forward<T>(initial));
    storage.changed.push_back(false);
    return static_cast<Index>(full_index);
  }

  template <typename T>
  void AddProperty(TypedIndex<Index, T>& index, T&& initial = T{}) {
    index.index = AddProperty<T>(std::forward<T>(initial));
  }

  template <typename T>
  [[nodiscard]] bool IsPropertyChanged(
      const TypedIndex<Index, T>& index) const noexcept {
    return IsPropertyChanged<T>(index.index);
  }

  template <typename T>
  [[nodiscard]] bool IsPropertyChanged(Index index) const noexcept {
    return GetTypeStorage<T>().changed[index];
  }

  template <typename T>
  [[nodiscard]] T& GetProperty(Index index) noexcept {
    return GetTypeStorage<T>().values[index];
  }

  template <typename T>
  [[nodiscard]] T& GetProperty(TypedIndex<Index, T> index) noexcept {
    return GetProperty<T>(index.index);
  }

  template <typename T>
  [[nodiscard]] const T& GetProperty(Index index) const noexcept {
    return GetTypeStorage<T>().values[index];
  }

  template <typename T>
  [[nodiscard]] const T& GetProperty(
      TypedIndex<Index, T> index) const noexcept {
    return GetProperty<T>(index.index);
  }

  template <typename T>
  void SetFlags(bool value) noexcept {
    auto& changed = GetTypeStorage<T>().changed;
    std::fill(changed.begin(), changed.end(), value);
  }

  void SetAllFlags(bool value) noexcept { (..., SetFlags<Types>(value)); }

  template <typename T>
  void SetChanged(Index index, bool value) noexcept {
    GetTypeStorage<T>().changed[index] = value;
  }

  template <typename T>
  void SetChanged(TypedIndex<Index, T> index, bool value) noexcept {
    SetChanged<T>(index.index, value);
  }

 private:
  template <typename T>
  [[nodiscard]] PropertyTypeStorage<T>& GetTypeStorage() noexcept {
    return std::get<PropertyTypeStorage<T>>(data);
  }

  template <typename T>
  [[nodiscard]] const PropertyTypeStorage<T>& GetTypeStorage() const noexcept {
    return std::get<PropertyTypeStorage<T>>(data);
  }

 private:
  std::tuple<PropertyTypeStorage<Types>...> data;
};
}  // namespace properties_container_impl

class ProgramProperties {
 public:
  using PropIndex = ui8;
  using PropsData = properties_container_impl::PropertiesContainer<
      PropIndex, float, bool, GlTextureWrapMode, GlTextureFilter, GlPolygonMode,
      glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

  template <typename T>
  using TypedIndex = properties_container_impl::TypedIndex<PropIndex, T>;
  using ColorIndex = TypedIndex<glm::vec4>;
  using FloatIndex = TypedIndex<float>;

 public:
  ProgramProperties();

  [[nodiscard]] bool Changed(auto index) const {
    return props_data_.IsPropertyChanged(index);
  }

  [[nodiscard]] auto& Get(auto index) { return props_data_.GetProperty(index); }

  [[nodiscard]] const auto& Get(auto index) const {
    return props_data_.GetProperty(index);
  }

  template <bool force = false, typename IndexType, typename F>
  void OnChange(IndexType index, F&& function) const noexcept {
    if constexpr (!force) {
      [[likely]] if (!props_data_.IsPropertyChanged(index)) { return; }
    }

    function(props_data_.GetProperty(index));
  }

  void MarkChanged(auto index, bool changed) {
    props_data_.SetChanged(index, changed);
  }
  void MarkAllChanged(bool changed) { props_data_.SetAllFlags(changed); }

  ColorIndex clear_color;
  ColorIndex global_color;
  ColorIndex tex_border_color;
  FloatIndex line_width;
  FloatIndex point_size;
  FloatIndex near_plane;
  FloatIndex far_plane;
  FloatIndex fov;
  TypedIndex<glm::vec2> tex_mult;
  TypedIndex<GlPolygonMode> polygon_mode;
  TypedIndex<GlTextureWrapMode> wrap_mode_s;
  TypedIndex<GlTextureWrapMode> wrap_mode_t;
  TypedIndex<GlTextureWrapMode> wrap_mode_r;
  TypedIndex<GlTextureFilter> min_filter;
  TypedIndex<GlTextureFilter> mag_filter;

  // Camera
  TypedIndex<glm::vec3> eye;
  TypedIndex<glm::vec3> look_at;
  TypedIndex<glm::vec3> camera_up;

 private:
  PropsData props_data_;
};

struct ParametersWidget {
  ParametersWidget(ProgramProperties* properties);

  void Update();

 private:
  void PolygonModeWidget();
  void ColorProperty(const char* title,
                     ProgramProperties::ColorIndex index) noexcept;
  void FloatProperty(const char* title, ProgramProperties::FloatIndex index,
                     float min, float max) noexcept;

  template <typename T>
  static constexpr ImGuiDataType_ CastDataType() noexcept {
    if constexpr (std::is_same_v<T, i8>) {
      return ImGuiDataType_S8;
    }
    if constexpr (std::is_same_v<T, ui8>) {
      return ImGuiDataType_U8;
    }
    if constexpr (std::is_same_v<T, i16>) {
      return ImGuiDataType_S16;
    }
    if constexpr (std::is_same_v<T, ui16>) {
      return ImGuiDataType_U16;
    }
    if constexpr (std::is_same_v<T, i32>) {
      return ImGuiDataType_S32;
    }
    if constexpr (std::is_same_v<T, ui32>) {
      return ImGuiDataType_U32;
    }
    if constexpr (std::is_same_v<T, i64>) {
      return ImGuiDataType_S64;
    }
    if constexpr (std::is_same_v<T, ui64>) {
      return ImGuiDataType_U64;
    }
    if constexpr (std::is_same_v<T, float>) {
      return ImGuiDataType_Float;
    }
    if constexpr (std::is_same_v<T, double>) {
      return ImGuiDataType_Double;
    }

    return ImGuiDataType_COUNT;
  }

  template <typename T, int N>
  void VectorProperty(const char* title,
                      ProgramProperties::TypedIndex<glm::vec<N, T>> index,
                      T min = std::numeric_limits<T>::lowest(),
                      T max = std::numeric_limits<T>::max()) noexcept {
    auto& value = Get(index);
    [[unlikely]] if (ImGui::DragScalarN(title, CastDataType<T>(),
                                        glm::value_ptr(value), N, 0.01f, &min,
                                        &max, "%.3f")) {
      MarkChanged(index);
    }
  }

  template <typename T, int C, int R>
  void MatrixProperty(const char* ppp,
                      ProgramProperties::TypedIndex<glm::mat<C, R, T>> index,
                      T min = std::numeric_limits<T>::lowest(),
                      T max = std::numeric_limits<T>::max()) noexcept {
    ImGui::PushID(ppp);
    auto& value = Get(index);
    bool changed = false;
    for (int row_index = 0; row_index < R; ++row_index) {
      auto row = glm::row(value, row_index);
      ImGui::PushID(row_index);
      const bool row_changed =
          ImGui::DragScalarN("", CastDataType<T>(), glm::value_ptr(row), C,
                             0.01f, &min, &max, "%.3f");
      ImGui::PopID();
      [[unlikely]] if (row_changed) {
        value = glm::row(value, row_index, row);
        changed = true;
      }
    }
    [[unlikely]] if (changed) { MarkChanged(index); }
    ImGui::PopID();
  }

  template <Enumeration T, size_t Extent>
  void EnumProperty(const char* title, ProgramProperties::TypedIndex<T> index,
                    std::span<std::string, Extent> values) {
    using U = std::underlying_type_t<T>;
    auto& value = Get(index);
    auto new_value = static_cast<U>(value);

    const char* selected_item = values[new_value].data();
    if (ImGui::BeginCombo(title, selected_item)) {
      for (size_t i = 0; i != values.size(); ++i) {
        const char* value_str = values[i].data();
        const bool is_selected = (selected_item == value_str);
        if (ImGui::Selectable(value_str, is_selected)) {
          new_value = static_cast<U>(i);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    [[unlikely]] if (value != static_cast<T>(new_value)) {
      value = static_cast<T>(new_value);
      MarkChanged(index);
    }
  }

  template <typename T, int N>
  [[nodiscard]] static bool VectorChanged(const glm::vec<N, T>& a,
                                          const glm::vec<N, T>& b) noexcept {
    bool result = false;
    for (int i = 0; i < N; ++i) {
      result |= FloatChanged(a[i], b[i]);
    }
    return result;
  }

  [[nodiscard]] static bool FloatChanged(float a, float b) noexcept {
    return std::abs(a - b) > 0.0001f;
  }

  [[nodiscard]] bool Changed(auto index) const {
    return properties_->Changed(index);
  }

  void MarkChanged(auto index) const { properties_->MarkChanged(index, true); }

  [[nodiscard]] auto& Get(auto index) { return properties_->Get(index); }

  [[nodiscard]] const auto& Get(auto index) const {
    return properties_->Get(index);
  }

 private:
  static constexpr size_t kNumPolygonModes =
      static_cast<size_t>(GlPolygonMode::Max);
  static constexpr size_t kNumTextureWrapModes =
      static_cast<size_t>(GlTextureWrapMode::Max);
  static constexpr size_t kNumTextureFilters =
      static_cast<size_t>(GlTextureFilter::Max);
  std::array<std::string, kNumPolygonModes> polygon_modes_;
  std::array<std::string, kNumTextureWrapModes> wrap_modes_;
  std::array<std::string, kNumTextureFilters> tex_filters_;

  ProgramProperties* properties_;
};
