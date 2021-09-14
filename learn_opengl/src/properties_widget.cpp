#include "properties_widget.hpp"

ParametersWidget::ParametersWidget() {
  props_data_.AddProperty(clear_color_idx_, {0.2f, 0.3f, 0.3f, 1.0f});
  props_data_.AddProperty(global_color_idx_, {0.5f, 0.0f, 0.0f, 1.0f});
  props_data_.AddProperty(border_color_idx_, {0.0f, 0.0f, 0.0f, 1.0f});
  props_data_.AddProperty(line_width_idx_, 1.0f);
  props_data_.AddProperty(point_size_idx_, 1.0f);
  props_data_.AddProperty(polygon_mode_idx_, GlPolygonMode::Fill);
  props_data_.AddProperty(wrap_mode_s_, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(wrap_mode_t_, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(wrap_mode_r_, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(tex_mult_, {1.0f, 1.0f});
  props_data_.AddProperty(transform_, glm::mat4(1.0));
  props_data_.AddProperty(min_filter_, GlTextureFilter::LinearMipmapLinear);
  props_data_.AddProperty(mag_filter_, GlTextureFilter::Linear);

  polygon_modes_[0] = "point";
  polygon_modes_[1] = "line";
  polygon_modes_[2] = "fill";

  wrap_modes_[0] = "clamp to edge";
  wrap_modes_[1] = "clamp to border";
  wrap_modes_[2] = "repeat";
  wrap_modes_[3] = "repeat mirrored";
  wrap_modes_[4] = "mirror clamp to edge";

  tex_filters_[0] = "Nearest";
  tex_filters_[1] = "Linear";
  tex_filters_[2] = "NearestMipmapNearest";
  tex_filters_[3] = "LinearMipmapNearest";
  tex_filters_[4] = "NearestMipmapLinear";
  tex_filters_[5] = "LinearMipmapLinear";
}

void ParametersWidget::Update() {
  ImGui::Begin("Settings");
  props_data_.SetAllFlags(false);
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
              static_cast<double>(ImGui::GetIO().Framerate));
  ColorProperty("clear color", clear_color_idx_);
  ColorProperty("draw color multiplier", global_color_idx_);
  ColorProperty("border color", border_color_idx_);
  PolygonModeWidget();
  if (ImGui::CollapsingHeader("Texture Sampling")) {
    EnumProperty("wrap s", wrap_mode_s_, std::span(wrap_modes_));
    EnumProperty("wrap t", wrap_mode_t_, std::span(wrap_modes_));
    EnumProperty("wrap r", wrap_mode_r_, std::span(wrap_modes_));
    EnumProperty("min filter", min_filter_, std::span(tex_filters_));
    EnumProperty("mag filter", mag_filter_,
                 std::span(tex_filters_).subspan(0, 2));
  }
  if (ImGui::CollapsingHeader("Texture coordinates multiplication")) {
    VectorProperty("mul", tex_mult_, 0.0f, 10.0f);
  }
  if (ImGui::CollapsingHeader("Transform")) {
    MatrixProperty(transform_, std::numeric_limits<float>::lowest(),
                   std::numeric_limits<float>::max());
  }
  ImGui::End();
}

void ParametersWidget::ColorProperty(const char* title,
                                     ColorIndex index) noexcept {
  auto& value = props_data_.GetProperty(index);
  auto new_value = value;
  ImGui::ColorEdit3(title, reinterpret_cast<float*>(&new_value));
  [[unlikely]] if (VectorChanged(new_value, value)) {
    value = new_value;
    props_data_.SetChanged(index, true);
  }
}

void ParametersWidget::FloatProperty(const char* title, FloatIndex index,
                                     float min, float max) noexcept {
  auto& value = props_data_.GetProperty(index);
  auto new_value = value;
  ImGui::SliderFloat(title, &new_value, min, max);
  [[unlikely]] if (FloatChanged(new_value, value)) {
    value = new_value;
    props_data_.SetChanged(index, true);
  }
}

void ParametersWidget::PolygonModeWidget() {
  if (ImGui::CollapsingHeader("Polygon Mode")) {
    EnumProperty("mode", polygon_mode_idx_, std::span(polygon_modes_));

    const GlPolygonMode mode = GetPolygonMode();
    if (mode == GlPolygonMode::Point) {
      FloatProperty("Point diameter", point_size_idx_, 1.0f, 100.0f);
    } else if (mode == GlPolygonMode::Line) {
      FloatProperty("Line width", line_width_idx_, 1.0f, 10.0f);
    }
  }
}