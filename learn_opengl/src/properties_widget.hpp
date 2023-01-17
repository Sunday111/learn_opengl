#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "opengl/gl_api.hpp"
#include "wrap/wrap_imgui.h"

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
      Eigen::Vector2f, Eigen::Vector3f, Eigen::Vector4f, Eigen::Matrix4f>;

  template <typename T>
  using TypedIndex = properties_container_impl::TypedIndex<PropIndex, T>;
  using ColorIndex = TypedIndex<Eigen::Vector4f>;
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
  ColorIndex tex_border_color;
  FloatIndex line_width;
  FloatIndex point_size;
  FloatIndex near_plane;
  FloatIndex far_plane;
  FloatIndex fov;
  TypedIndex<GlPolygonMode> polygon_mode;
  TypedIndex<GlTextureWrapMode> wrap_mode_s;
  TypedIndex<GlTextureWrapMode> wrap_mode_t;
  TypedIndex<GlTextureWrapMode> wrap_mode_r;
  TypedIndex<GlTextureFilter> min_filter;
  TypedIndex<GlTextureFilter> mag_filter;

  // Camera
  TypedIndex<Eigen::Vector3f> eye;
  TypedIndex<Eigen::Vector3f> look_at;
  TypedIndex<Eigen::Vector3f> camera_up;

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

  template <typename T, int N>
  void VectorProperty(
      const char* title,
      ProgramProperties::TypedIndex<Eigen::Matrix<T, N, 1>> index,
      T min = std::numeric_limits<T>::lowest(),
      T max = std::numeric_limits<T>::max()) noexcept {
    auto& value = Get(index);
    [[unlikely]] if (ImGui::DragScalarN(title, CastDataType<T>(), value.data(),
                                        N, 0.01f, &min, &max, "%.3f")) {
      MarkChanged(index);
    }
  }

  // TODO: convert to eigen
  template <typename T, int C, int R>
  void MatrixProperty(
      const char* ppp,
      ProgramProperties::TypedIndex<Eigen::Matrix<T, R, C>> index,
      T min = std::numeric_limits<T>::lowest(),
      T max = std::numeric_limits<T>::max()) noexcept {
    ImGui::PushID(ppp);
    Eigen::Matrix<T, R, C>& value = Get(index);
    bool changed = false;
    for (int row_index = 0; row_index < R; ++row_index) {
      value.block();
      auto row_view = value.template block<1, C>(row_index, 0);
      Eigen::Matrix<T, R, 1> row = row_view;
      ImGui::PushID(row_index);
      const bool row_changed = ImGui::DragScalarN(
          "", CastDataType<T>(), row.data(), C, 0.01f, &min, &max, "%.3f");
      ImGui::PopID();
      [[unlikely]] if (row_changed) {
        row_view = row;
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
  [[nodiscard]] static bool VectorChanged(
      const Eigen::Matrix<T, N, 1>& a,
      const Eigen::Matrix<T, N, 1>& b) noexcept {
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
