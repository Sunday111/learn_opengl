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
      PropIndex, float, bool, GlTextureWrapMode, GlTextureFilter, GlPolygonMode,
      glm::vec2, glm::vec4, glm::mat4>;

  template <typename T>
  using TypedIndex = properties_container_impl::TypedIndex<PropIndex, T>;
  using ColorIndex = TypedIndex<glm::vec4>;
  using FloatIndex = TypedIndex<float>;

  ParametersWidget();

  void Update();

  // Near plane

  [[nodiscard]] auto NearPlaneIdx() const noexcept { return near_plane_; }

  [[nodiscard]] bool NearPlaneChanged() const noexcept {
    return props_data_.PropertyChanged(NearPlaneIdx());
  }

  [[nodiscard]] float GetNearPlane() const noexcept {
    return props_data_.GetProperty(NearPlaneIdx());
  }

  // Far plane

  [[nodiscard]] auto FarPlaneIdx() const noexcept { return far_plane_; }

  [[nodiscard]] bool FarPlaneChanged() const noexcept {
    return props_data_.PropertyChanged(FarPlaneIdx());
  }

  [[nodiscard]] float GetFarPlane() const noexcept {
    return props_data_.GetProperty(FarPlaneIdx());
  }

  // FOV

  [[nodiscard]] auto FovIdx() const noexcept { return fov_; }

  [[nodiscard]] bool FovChanged() const noexcept {
    return props_data_.PropertyChanged(FovIdx());
  }

  [[nodiscard]] float GetFov() const noexcept {
    return props_data_.GetProperty(FovIdx());
  }

  // Min filter

  [[nodiscard]] auto MinFilterIdx() const noexcept { return min_filter_; }

  [[nodiscard]] bool MinFilterChanged() const noexcept {
    return props_data_.PropertyChanged(MinFilterIdx());
  }

  [[nodiscard]] GlTextureFilter GetMinFilter() const noexcept {
    return props_data_.GetProperty(MinFilterIdx());
  }

  // Mag filter

  [[nodiscard]] auto MagFilterIdx() const noexcept { return mag_filter_; }

  [[nodiscard]] bool MagFilterChanged() const noexcept {
    return props_data_.PropertyChanged(MagFilterIdx());
  }

  [[nodiscard]] GlTextureFilter GetMagFilter() const noexcept {
    return props_data_.GetProperty(MagFilterIdx());
  }

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

  // Model
  [[nodiscard]] auto ModelMtxIdx() const noexcept { return model_; }

  [[nodiscard]] bool ModelMtxChanged() const noexcept {
    return props_data_.PropertyChanged(ModelMtxIdx());
  }

  [[nodiscard]] const glm::mat4& GetModelMtx() const noexcept {
    return props_data_.GetProperty(ModelMtxIdx());
  }

  // View
  [[nodiscard]] auto ViewMtxIdx() const noexcept { return view_; }

  [[nodiscard]] bool ViewMtxChanged() const noexcept {
    return props_data_.PropertyChanged(ViewMtxIdx());
  }

  [[nodiscard]] const glm::mat4& GetViewMtx() const noexcept {
    return props_data_.GetProperty(ViewMtxIdx());
  }

  // Proj
  [[nodiscard]] auto ProjMtxIdx() const noexcept { return proj_; }

  [[nodiscard]] bool ViewProjChanged() const noexcept {
    return props_data_.PropertyChanged(ProjMtxIdx());
  }

  [[nodiscard]] const glm::mat4& GetProjMtx() const noexcept {
    return props_data_.GetProperty(ProjMtxIdx());
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
  void OnChange(IndexType index, F&& function) const noexcept {
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
  void VectorProperty(const char* title, TypedIndex<glm::vec<N, T>> index,
                      float min, float max) noexcept {
    auto& value = props_data_.GetProperty(index);
    [[unlikely]] if (ImGui::DragScalarN(title, CastDataType<T>(),
                                        glm::value_ptr(value), N, 0.01f, &min,
                                        &max, "%.3f")) {
      props_data_.SetChanged(index, true);
    }
  }

  template <typename T, int C, int R>
  void MatrixProperty(const char* ppp, TypedIndex<glm::mat<C, R, T>> index,
                      T min = std::numeric_limits<T>::lowest(),
                      T max = std::numeric_limits<T>::max()) noexcept {
    ImGui::PushID(ppp);
    auto& value = props_data_.GetProperty(index);
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
    [[unlikely]] if (changed) { props_data_.SetChanged(index, true); }
    ImGui::PopID();
  }

  template <Enumeration T, size_t Extent>
  void EnumProperty(const char* title, TypedIndex<T> index,
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

  template <Enumeration T>
  [[nodiscard]] T GetEnumProperty(TypedIndex<T> index) const noexcept {
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
  static constexpr size_t kNumTextureFilters =
      static_cast<size_t>(GlTextureFilter::Max);
  std::array<std::string, kNumPolygonModes> polygon_modes_;
  std::array<std::string, kNumTextureWrapModes> wrap_modes_;
  std::array<std::string, kNumTextureFilters> tex_filters_;

  PropsData props_data_;
  ColorIndex clear_color_idx_;
  ColorIndex global_color_idx_;
  ColorIndex border_color_idx_;
  FloatIndex line_width_idx_;
  FloatIndex point_size_idx_;
  FloatIndex near_plane_;
  FloatIndex far_plane_;
  FloatIndex fov_;
  TypedIndex<glm::vec2> tex_mult_;
  TypedIndex<GlPolygonMode> polygon_mode_idx_;
  TypedIndex<GlTextureWrapMode> wrap_mode_s_;
  TypedIndex<GlTextureWrapMode> wrap_mode_t_;
  TypedIndex<GlTextureWrapMode> wrap_mode_r_;
  TypedIndex<glm::mat4> model_;
  TypedIndex<glm::mat4> view_;
  TypedIndex<glm::mat4> proj_;
  TypedIndex<GlTextureFilter> min_filter_;
  TypedIndex<GlTextureFilter> mag_filter_;
};
