#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "gl_api.hpp"
#include "include_imgui.h"

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
  [[nodiscard]] bool PropertyChanged(
      const TypedIndex<Index, T>& index) const noexcept {
    return PropertyChanged<T>(index.index);
  }

  template <typename T>
  [[nodiscard]] bool PropertyChanged(Index index) const noexcept {
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

struct ParametersWidget {
  using PropIndex = ui8;
  using PropsData = properties_container_impl::PropertiesContainer<
      PropIndex, glm::vec2, glm::vec4, float, GlPolygonMode, bool,
      GlTextureWrapMode>;

  template <typename T>
  using TypedIndex = properties_container_impl::TypedIndex<PropIndex, T>;
  using ColorIndex = TypedIndex<glm::vec4>;
  using FloatIndex = TypedIndex<float>;

  template <typename T, typename Enable = std::enable_if_t<std::is_enum_v<T>>>
  using EnumIndex = TypedIndex<T>;

  ParametersWidget();

  void Update();

  // Point Size Changed

  [[nodiscard]] auto GetPointSizeIndex() const noexcept {
    return point_size_idx_;
  }

  [[nodiscard]] bool PointSizeChanged() const noexcept {
    return props_data_.PropertyChanged(point_size_idx_);
  }

  [[nodiscard]] float GetPointSize() const noexcept {
    return props_data_.GetProperty(point_size_idx_);
  }

  // Line Width

  [[nodiscard]] auto GetLineWidthIndex() const noexcept {
    return line_width_idx_;
  }

  [[nodiscard]] bool LineWidthChanged() const noexcept {
    return props_data_.PropertyChanged(line_width_idx_);
  }

  [[nodiscard]] float GetLineWidth() const noexcept {
    return props_data_.GetProperty(line_width_idx_);
  }

  // Polygon Mode

  [[nodiscard]] auto GetPolygonModeIndex() const noexcept {
    return polygon_mode_idx_;
  }

  [[nodiscard]] bool PolygonModeChanged() const noexcept {
    return props_data_.PropertyChanged(polygon_mode_idx_);
  }

  [[nodiscard]] GlPolygonMode GetPolygonMode() const noexcept {
    return GetEnumProperty<GlPolygonMode>(polygon_mode_idx_);
  }

  // Clear Color

  [[nodiscard]] auto GetClearColorIndex() const noexcept {
    return clear_color_idx_;
  }

  [[nodiscard]] bool ClearColorChanged() const noexcept {
    return props_data_.PropertyChanged(clear_color_idx_);
  }

  [[nodiscard]] const glm::vec4& GetClearColor() const noexcept {
    return props_data_.GetProperty(clear_color_idx_);
  }

  // Global Color

  [[nodiscard]] auto GetGlobalColorIndex() const noexcept {
    return global_color_idx_;
  }

  [[nodiscard]] bool GlobalColorChanged() const noexcept {
    return props_data_.PropertyChanged(global_color_idx_);
  }

  [[nodiscard]] const glm::vec4& GetGlobalColor() const noexcept {
    return props_data_.GetProperty(global_color_idx_);
  }

  // Border Color

  [[nodiscard]] auto GetBorderColorIndex() const noexcept {
    return border_color_idx_;
  }

  [[nodiscard]] bool BorderColorChanged() const noexcept {
    return props_data_.PropertyChanged(GetBorderColorIndex());
  }

  [[nodiscard]] const glm::vec4& GetBorderColor() const noexcept {
    return props_data_.GetProperty(GetBorderColorIndex());
  }

  // Get texture coordinate multiplier
  [[nodiscard]] glm::vec2 GetTexCoordMultiplier() const noexcept {
    return props_data_.GetProperty(tex_mult_);
  }

  [[nodiscard]] auto GetTexCoordMultiplierIndex() const noexcept {
    return tex_mult_;
  }

  // Get wrap mode
  [[nodiscard]] auto GetTextureWrapModeSIndex() const noexcept {
    return wrap_mode_s_;
  }

  [[nodiscard]] auto GetTextureWrapModeTIndex() const noexcept {
    return wrap_mode_t_;
  }

  [[nodiscard]] auto GetTextureWrapModeRIndex() const noexcept {
    return wrap_mode_r_;
  }

  // Generic
  template <bool force = false, typename IndexType, typename F>
  void CheckPropertyChange(IndexType index, F&& function) const noexcept {
    if constexpr (!force) {
      [[likely]] if (!props_data_.PropertyChanged(index)) { return; }
    }

    function(props_data_.GetProperty(index));
  }

  template <typename IndexType>
  auto GetProperty(IndexType index) const noexcept {
    return props_data_.GetProperty(index);
  }

 private:
  void PolygonModeWidget();
  void ColorProperty(const char* title, ColorIndex index) noexcept;
  void FloatProperty(const char* title, FloatIndex index, float min,
                     float max) noexcept;

  template <typename T, int N>
  void VectorProperty(const char* title, TypedIndex<glm::vec<N, T>> index,
                      float min, float max) noexcept {
    auto& value = props_data_.GetProperty(index);
    auto new_value = value;
    ImGui::DragFloat2(title, &new_value.x, 0.01f, min, max);
    [[unlikely]] if (VectorChanged(new_value, value)) {
      value = new_value;
      props_data_.SetChanged(index, true);
    }
  }

  template <typename T, size_t Extent>
  void EnumProperty(const char* title, EnumIndex<T> index,
                    std::span<std::string, Extent> values) {
    using U = std::underlying_type_t<T>;
    auto& value = props_data_.GetProperty(index);
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
      props_data_.SetChanged(index, true);
    }
  }

  template <typename T>
  [[nodiscard]] T GetEnumProperty(EnumIndex<T> index) const noexcept {
    return static_cast<T>(props_data_.GetProperty(index));
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

 private:
  static constexpr size_t kNumPolygonModes =
      static_cast<size_t>(GlPolygonMode::Max);
  static constexpr size_t kNumTextureWrapModes =
      static_cast<size_t>(GlTextureWrapMode::Max);
  std::array<std::string, kNumTextureWrapModes> polygon_modes_;
  std::array<std::string, kNumTextureWrapModes> wrap_modes_;

  PropsData props_data_;
  ColorIndex clear_color_idx_;
  ColorIndex global_color_idx_;
  ColorIndex border_color_idx_;
  FloatIndex line_width_idx_;
  FloatIndex point_size_idx_;
  TypedIndex<glm::vec2> tex_mult_;
  EnumIndex<GlPolygonMode> polygon_mode_idx_;
  EnumIndex<GlTextureWrapMode> wrap_mode_s_;
  EnumIndex<GlTextureWrapMode> wrap_mode_t_;
  EnumIndex<GlTextureWrapMode> wrap_mode_r_;
};
