#include "properties_widget.hpp"

ProgramProperties::ProgramProperties() {
  props_data_.AddProperty(clear_color, {0.2f, 0.3f, 0.3f, 1.0f});
  props_data_.AddProperty(tex_border_color, {0.0f, 0.0f, 0.0f, 1.0f});
  props_data_.AddProperty(line_width, 1.0f);
  props_data_.AddProperty(point_size, 1.0f);
  props_data_.AddProperty(polygon_mode, GlPolygonMode::Fill);
  props_data_.AddProperty(wrap_mode_s, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(wrap_mode_t, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(wrap_mode_r, GlTextureWrapMode::Repeat);
  props_data_.AddProperty(eye, {3.0f, 3.0f, 3.0f});
  props_data_.AddProperty(look_at, {0.0f, 0.0f, 0.0f});
  props_data_.AddProperty(camera_up, {0.0f, 0.0f, 1.0f});
  props_data_.AddProperty(min_filter, GlTextureFilter::LinearMipmapLinear);
  props_data_.AddProperty(mag_filter, GlTextureFilter::Linear);
  props_data_.AddProperty(near_plane, 0.1f);
  props_data_.AddProperty(far_plane, 100.0f);
  props_data_.AddProperty(fov, 45.0f);
}

ParametersWidget::ParametersWidget(ProgramProperties* properties)
    : properties_(properties) {
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
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
              static_cast<double>(ImGui::GetIO().Framerate));
  ColorProperty("clear color", properties_->clear_color);
  ColorProperty("border color", properties_->tex_border_color);
  PolygonModeWidget();
  if (ImGui::CollapsingHeader("Texture Sampling")) {
    EnumProperty("wrap s", properties_->wrap_mode_s, std::span(wrap_modes_));
    EnumProperty("wrap t", properties_->wrap_mode_t, std::span(wrap_modes_));
    EnumProperty("wrap r", properties_->wrap_mode_r, std::span(wrap_modes_));
    EnumProperty("min filter", properties_->min_filter,
                 std::span(tex_filters_));
    EnumProperty("mag filter", properties_->mag_filter,
                 std::span(tex_filters_).subspan(0, 2));
  }
  if (ImGui::CollapsingHeader("Transformations")) {
    ImGui::Text("Camera");
    VectorProperty("eye", properties_->eye);
    VectorProperty("look_at", properties_->look_at);
    VectorProperty("camera_up", properties_->camera_up);
    ImGui::Separator();
    ImGui::Text("projection");
    FloatProperty("near plane", properties_->near_plane, 0.0f, 1000.0f);
    FloatProperty("far plane", properties_->far_plane, 0.0f, 1000.0f);
    FloatProperty("fov", properties_->fov, 0.1f, 89.0f);
  }
  ImGui::End();
}

void ParametersWidget::ColorProperty(
    const char* title, ProgramProperties::ColorIndex index) noexcept {
  auto& value = Get(index);
  auto new_value = value;
  ImGui::ColorEdit3(title, reinterpret_cast<float*>(&new_value));
  [[unlikely]] if (VectorChanged(new_value, value)) {
    value = new_value;
    MarkChanged(index);
  }
}

void ParametersWidget::FloatProperty(const char* title,
                                     ProgramProperties::FloatIndex index,
                                     float min, float max) noexcept {
  auto& value = Get(index);
  auto new_value = value;
  ImGui::SliderFloat(title, &new_value, min, max);
  [[unlikely]] if (FloatChanged(new_value, value)) {
    value = new_value;
    MarkChanged(index);
  }
}

void ParametersWidget::PolygonModeWidget() {
  if (ImGui::CollapsingHeader("Polygon Mode")) {
    EnumProperty("mode", properties_->polygon_mode, std::span(polygon_modes_));

    const GlPolygonMode mode = Get(properties_->polygon_mode);
    if (mode == GlPolygonMode::Point) {
      FloatProperty("Point diameter", properties_->point_size, 1.0f, 100.0f);
    } else if (mode == GlPolygonMode::Line) {
      FloatProperty("Line width", properties_->line_width, 1.0f, 10.0f);
    }
  }
}