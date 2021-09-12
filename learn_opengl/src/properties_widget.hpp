#pragma once

#include <stdexcept>
#include <type_traits>

#include "gl_api.hpp"

namespace properties_container_impl {
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
  [[nodiscard]] bool PropertyChanged(Index index) const noexcept {
    return GetTypeStorage<T>().changed[index];
  }

  template <typename T>
  [[nodiscard]] T& GetProperty(Index index) noexcept {
    return GetTypeStorage<T>().values[index];
  }

  template <typename T>
  [[nodiscard]] const T& GetProperty(Index index) const noexcept {
    return GetTypeStorage<T>().values[index];
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
  using PolygonModeU = std::underlying_type_t<GlPolygonMode>;
  using PropIndex = ui8;
  using PropsData =
      properties_container_impl::PropertiesContainer<PropIndex, glm::vec4,
                                                     float, PolygonModeU, bool>;

  template <typename T, size_t N>
  union BitfieldHelper {
    T fill;
    T value : N;
  };

  ParametersWidget() {
    clear_color_idx_ =
        props_data_.AddProperty<glm::vec4>({0.2f, 0.3f, 0.3f, 1.0f});
    global_color_idx_ =
        props_data_.AddProperty<glm::vec4>({0.5f, 0.0f, 0.0f, 1.0f});
    border_color_idx_ =
        props_data_.AddProperty<glm::vec4>({0.0f, 0.0f, 0.0f, 1.0f});
    line_width_idx_ = props_data_.AddProperty<float>(1.0f);
    point_size_idx_ = props_data_.AddProperty<float>(1.0f);
    polygon_mode_idx_ = props_data_.AddProperty<PolygonModeU>(2);

    polygon_modes_[0] = "point";
    polygon_modes_[1] = "line";
    polygon_modes_[2] = "fill";
  }

  void ColorProperty(const char* title, PropIndex index) noexcept {
    auto& value = props_data_.GetProperty<glm::vec4>(index);
    auto new_value = value;
    ImGui::ColorEdit3(title, reinterpret_cast<float*>(&new_value));
    [[unlikely]] if (ColorChanged(new_value, value)) {
      value = new_value;
      props_data_.SetChanged<glm::vec4>(index, true);
    }
  }

  void FloatProperty(const char* title, PropIndex index, float min,
                     float max) noexcept {
    auto& value = props_data_.GetProperty<float>(index);
    auto new_value = value;
    ImGui::SliderFloat(title, &new_value, min, max);
    [[unlikely]] if (FloatChanged(new_value, value)) {
      value = new_value;
      props_data_.SetChanged<float>(index, true);
    }
  }

  template <typename T, size_t Extent>
  void EnumProperty(const char* title, PropIndex index,
                    std::span<std::string, Extent> values) {
    using U = std::underlying_type_t<T>;
    auto& value = props_data_.GetProperty<U>(index);
    auto new_value = value;

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

    [[unlikely]] if (value != new_value) {
      value = new_value;
      props_data_.SetChanged<U>(index, true);
    }
  }

  void ClearColorWidget() { ColorProperty("clear color", clear_color_idx_); }

  void MultiplyColorWidget() {
    ColorProperty("draw color multiplier", global_color_idx_);
  }

  void BorderColorWidget() { ColorProperty("border color", border_color_idx_); }

  void FPSWidget() {
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
                static_cast<double>(ImGui::GetIO().Framerate));
  }

  void PolygonModeWidget() {
    if (ImGui::CollapsingHeader("Polygon Mode")) {
      EnumProperty<GlPolygonMode>("mode", polygon_mode_idx_,
                                  std::span(polygon_modes_));

      const GlPolygonMode mode = GetPolygonMode();
      if (mode == GlPolygonMode::Point) {
        FloatProperty("Point diameter", point_size_idx_, 1.0f, 100.0f);
      } else if (mode == GlPolygonMode::Line) {
        FloatProperty("Line width", line_width_idx_, 1.0f, 10.0f);
      }
    }
  }

  void Update() {
    ImGui::Begin("Settings");
    props_data_.SetAllFlags(false);
    FPSWidget();
    ClearColorWidget();
    MultiplyColorWidget();
    BorderColorWidget();
    PolygonModeWidget();
    ImGui::End();
  }

  [[nodiscard]] bool PointSizeChanged() const noexcept {
    return props_data_.PropertyChanged<float>(point_size_idx_);
  }

  [[nodiscard]] float GetPointSize() const noexcept {
    return props_data_.GetProperty<float>(point_size_idx_);
  }

  [[nodiscard]] bool LineWidthChanged() const noexcept {
    return props_data_.PropertyChanged<float>(line_width_idx_);
  }

  [[nodiscard]] float GetLineWidth() const noexcept {
    return props_data_.GetProperty<float>(line_width_idx_);
  }

  [[nodiscard]] bool PolygonModeChanged() const noexcept {
    return props_data_.PropertyChanged<PolygonModeU>(polygon_mode_idx_);
  }

  [[nodiscard]] GlPolygonMode GetPolygonMode() const noexcept {
    const auto u = props_data_.GetProperty<PolygonModeU>(polygon_mode_idx_);
    switch (u) {
      case 0:
        return GlPolygonMode::Point;
      case 1:
        return GlPolygonMode::Line;
      default:
        return GlPolygonMode::Fill;
    }
  }
  [[nodiscard]] static bool ColorChanged(const glm::vec4& a,
                                         const glm::vec4& b) noexcept {
    return FloatChanged(a.r, b.r) | FloatChanged(a.g, b.g) |
           FloatChanged(a.b, b.b) | FloatChanged(a.a, b.a);
  }

  [[nodiscard]] static bool FloatChanged(float a, float b) noexcept {
    return std::abs(a - b) > 0.0001f;
  }

  [[nodiscard]] const glm::vec4& GetClearColor() const noexcept {
    return props_data_.GetProperty<glm::vec4>(clear_color_idx_);
  }

  [[nodiscard]] const glm::vec4& GetGlobalColor() const noexcept {
    return props_data_.GetProperty<glm::vec4>(global_color_idx_);
  }

  [[nodiscard]] const glm::vec4& GetBorderColor() const noexcept {
    return props_data_.GetProperty<glm::vec4>(border_color_idx_);
  }

  std::array<std::string, 3> polygon_modes_;

  PropsData props_data_;
  PropIndex clear_color_idx_;
  PropIndex global_color_idx_;
  PropIndex border_color_idx_;
  PropIndex line_width_idx_;
  PropIndex point_size_idx_;
  PropIndex polygon_mode_idx_;
};
