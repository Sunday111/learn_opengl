#include "render_system.hpp"

#include "components/camera_component.hpp"
#include "components/lights/directional_light_component.hpp"
#include "components/lights/point_light_component.hpp"
#include "components/lights/spot_light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/transform_component.hpp"
#include "entities/entity.hpp"
#include "opengl/debug/annotations.hpp"
#include "reflection/eigen_reflect.hpp"
#include "spdlog/spdlog.h"
#include "texture/texture_manager.hpp"
#include "window.hpp"
#include "world.hpp"

auto GetMaterialUniform(Shader& s) {
  MaterialUniform u;
  u.diffuse = s.GetUniform("material.diffuse");
  u.specular = s.GetUniform("material.specular");
  u.shininess = s.GetUniform("material.shininess");
  return u;
}

static auto GetArrayUniform(const Shader& s, std::string_view array_name,
                            size_t index, std::string_view property_name) {
  return s.GetUniform(
      fmt::format("{}[{}].{}", array_name, index, property_name));
}

auto GetPointLightUniform(Shader& s, size_t index) {
  auto get = [&](std::string_view name) {
    return GetArrayUniform(s, "pointLights", index, name);
  };

  PointLightUniform u;
  u.location = get("location");
  u.ambient = get("ambient");
  u.diffuse = get("diffuse");
  u.specular = get("specular");
  u.constant = get("attenuation.constant");
  u.linear = get("attenuation.linear");
  u.quadratic = get("attenuation.quadratic");
  return u;
}

auto GetDirectionalLightUniform(Shader& s, size_t index) {
  auto get = [&](std::string_view name) {
    return GetArrayUniform(s, "directionalLights", index, name);
  };

  DirectionalLightUniform u;
  u.direction = get("direction");
  u.ambient = get("ambient");
  u.diffuse = get("diffuse");
  u.specular = get("specular");
  return u;
}

auto GetSpotLightUniform(Shader& s, size_t index) {
  auto get = [&](std::string_view name) {
    return GetArrayUniform(s, "spotLights", index, name);
  };

  SpotLightUniform u;
  u.location = get("location");
  u.direction = get("direction");
  u.diffuse = get("diffuse");
  u.specular = get("specular");
  u.innerAngle = get("innerAngle");
  u.outerAngle = get("outerAngle");
  u.constant = get("attenuation.constant");
  u.linear = get("attenuation.linear");
  u.quadratic = get("attenuation.quadratic");
  return u;
}

void PrintUniform(const PointLightUniform& u) {
  spdlog::info(u.location.name.GetView());
  spdlog::info(u.ambient.name.GetView());
  spdlog::info(u.diffuse.name.GetView());
  spdlog::info(u.specular.name.GetView());
  spdlog::info(u.constant.name.GetView());
  spdlog::info(u.linear.name.GetView());
  spdlog::info(u.quadratic.name.GetView());
}

void PrintUniform(const DirectionalLightUniform& u) {
  spdlog::info(u.direction.name.GetView());
  spdlog::info(u.ambient.name.GetView());
  spdlog::info(u.diffuse.name.GetView());
  spdlog::info(u.specular.name.GetView());
}

void PrintUniform(const SpotLightUniform& u) {
  spdlog::info(u.location.name.GetView());
  spdlog::info(u.direction.name.GetView());
  spdlog::info(u.diffuse.name.GetView());
  spdlog::info(u.specular.name.GetView());
  spdlog::info(u.innerAngle.name.GetView());
  spdlog::info(u.outerAngle.name.GetView());
  spdlog::info(u.constant.name.GetView());
  spdlog::info(u.linear.name.GetView());
  spdlog::info(u.quadratic.name.GetView());
}

template <typename T, size_t Extent>
void PrintUniforms(std::span<T, Extent> uniforms) {
  for (auto& uniform : uniforms) {
    PrintUniform(uniform);
  }
}

void ApplyUniforms(PointLightUniform& u, Shader& s,
                   TransformComponent& transform, PointLightComponent& light) {
  {
    Eigen::Transform<float, 3, Eigen::Affine> tr(transform.transform);
    Eigen::Vector3f l = tr.translation();
    s.SetUniform(u.location, l);
  }

  s.SetUniform(u.ambient, light.ambient);
  s.SetUniform(u.diffuse, light.diffuse);
  s.SetUniform(u.specular, light.specular);
  s.SetUniform(u.constant, light.attenuation.constant);
  s.SetUniform(u.linear, light.attenuation.linear);
  s.SetUniform(u.quadratic, light.attenuation.quadratic);
}

void ApplyUniforms(SpotLightUniform& u, Shader& s,
                   TransformComponent& transform, SpotLightComponent& light) {
  {
    Eigen::Vector3f dir(0.0f, 0.0f, -1.0f);
    s.SetUniform(u.location, transform.GetTranslation());
    Eigen::Vector3f direction_mtx =
        (dir.transpose() * transform.GetRotationMtx()).transpose();
    s.SetUniform(u.direction, direction_mtx);
  }

  s.SetUniform(u.diffuse, light.diffuse);
  s.SetUniform(u.specular, light.specular);
  s.SetUniform(u.innerAngle, light.innerAngle);
  s.SetUniform(u.outerAngle, light.outerAngle);
  s.SetUniform(u.constant, light.attenuation.constant);
  s.SetUniform(u.linear, light.attenuation.linear);
  s.SetUniform(u.quadratic, light.attenuation.quadratic);
}

void ApplyUniforms(DirectionalLightUniform& u, Shader& s,
                   TransformComponent& transform,
                   DirectionalLightComponent& light) {
  {
    Eigen::Vector3f dir(1.0f, 0.0f, 0.0f);
    dir = (dir.transpose() * transform.GetRotationMtx()).transpose();
    s.SetUniform(u.direction, dir);
  }

  s.SetUniform(u.ambient, light.ambient);
  s.SetUniform(u.diffuse, light.diffuse);
  s.SetUniform(u.specular, light.specular);
}

template <typename UniformType, typename LightComponent, typename UniformGetter>
void SetLightsArrayUniform(
    Shader& shader, DefineHandle& define, std::vector<UniformType>& uniforms,
    std::vector<std::pair<TransformComponent*, LightComponent*>>& lights,
    const std::pair<TransformComponent*, LightComponent*>& default_light,
    UniformGetter uniform_getter) {
  auto num_uniforms = static_cast<size_t>(shader.GetDefineValue<int>(define));

  [[unlikely]] if (lights.size() > num_uniforms) {
    int new_define = static_cast<int>(lights.size());
    shader.SetDefineValue(define, new_define);
    num_uniforms = static_cast<size_t>(new_define);
    shader.Compile();
    shader.Use();
  }

  // Refresh uniforms array if number changed
  [[unlikely]] if (num_uniforms != uniforms.size()) {
    uniforms.resize(num_uniforms);
    for (size_t idx = 0; idx < num_uniforms; ++idx) {
      uniforms[idx] = uniform_getter(shader, idx);
    }
    // PrintUniforms(std::span{uniforms});
  }

  // Apply actual lights
  size_t uniform_index = 0;
  while ((uniform_index < lights.size()) && (uniform_index < num_uniforms)) {
    auto [t, l] = lights[uniform_index];
    ApplyUniforms(uniforms[uniform_index], shader, *t, *l);
    ++uniform_index;
  }

  // Apply defaults if there are unused slots
  auto [t, l] = default_light;
  while (uniform_index < num_uniforms) {
    ApplyUniforms(uniforms[uniform_index], shader, *t, *l);
    ++uniform_index;
  }
}

RenderSystem::RenderSystem(TextureManager& texture_manager)
    : texture_manager_(&texture_manager) {
  shader_ = std::make_shared<Shader>("simple.shader.json");
  outline_shader_ = std::make_shared<Shader>("outline.shader.json");
  shader_->Use();

  container_diffuse_ = texture_manager.GetTexture("container.texture.json");
  container_specular_ =
      texture_manager.GetTexture("container_specular.texture.json");

  default_point_light_.ambient = Eigen::Vector3f::Zero();
  default_point_light_.diffuse = Eigen::Vector3f::Zero();
  default_point_light_.specular = Eigen::Vector3f::Zero();
  default_directional_light_.ambient = Eigen::Vector3f::Zero();
  default_directional_light_.diffuse = Eigen::Vector3f::Zero();
  default_directional_light_.specular = Eigen::Vector3f::Zero();
  default_spot_light_.diffuse = Eigen::Vector3f::Zero();
  default_spot_light_.specular = Eigen::Vector3f::Zero();
  def_num_point_lights_ = shader_->GetDefine("cv_num_point_lights");
  def_num_directional_lights_ = shader_->GetDefine("cv_num_directional_lights");
  def_num_spot_lights_ = shader_->GetDefine("cv_num_spot_lights");

  material_uniform_ = GetMaterialUniform(*shader_);

  model_uniform_ = shader_->GetUniform("model");
  view_uniform_ = shader_->GetUniform("view");
  projection_uniform_ = shader_->GetUniform("projection");
  view_location_uniform_ = shader_->GetUniform("viewLocation");
  tex_multiplier_uniform_ = shader_->GetUniform("texCoordMultiplier");

  outline_model_uniform_ = shader_->GetUniform("model");
  outline_view_uniform_ = shader_->GetUniform("view");
  outline_projection_uniform_ = shader_->GetUniform("projection");

  shader_->SetUniform(material_uniform_.diffuse, container_diffuse_);
  shader_->SetUniform(material_uniform_.specular, container_specular_);
  shader_->SetUniform(material_uniform_.shininess, 32.0f);
  shader_->SetUniform(tex_multiplier_uniform_, Eigen::Vector2f{1.0f, 1.0f});
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::ApplyLights() {
  SetLightsArrayUniform(
      *shader_, def_num_point_lights_, point_light_uniforms_, point_lights_,
      {&default_transform_, &default_point_light_}, GetPointLightUniform);

  SetLightsArrayUniform(*shader_, def_num_directional_lights_,
                        directional_light_uniforms_, directional_lights_,
                        {&default_transform_, &default_directional_light_},
                        GetDirectionalLightUniform);

  SetLightsArrayUniform(
      *shader_, def_num_spot_lights_, spot_light_uniforms_, spot_lights_,
      {&default_transform_, &default_spot_light_}, GetSpotLightUniform);
}

void RenderSystem::Render(Window& window, World& world, Entity* selected) {
  OpenGl::Viewport(0, 0, static_cast<GLsizei>(window.GetWidth()),
                   static_cast<GLsizei>(window.GetHeight()));

  glDisable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

  {
    ScopeAnnotation annot_render_("Render world");
    shader_->Use();
    shader_->SetUniform(view_uniform_, window.GetView());
    shader_->SetUniform(view_location_uniform_, window.GetCamera()->eye);
    shader_->SetUniform(projection_uniform_, window.GetProjection());

    ApplyLights();
    shader_->SendUniforms();

    world.ForEachEntity([&](Entity& entity) {
      const bool is_selected = (&entity == selected);
      // don't update stencil buffer for not selected objects
      glStencilMask(is_selected ? 0xFF : 0x00);

      entity.ForEachComp<TransformComponent>(
          [&](TransformComponent& transform_component) {
            auto t = transform_component.transform;
            shader_->SetUniform(model_uniform_, t);
            shader_->SendUniform(model_uniform_);
          });

      entity.ForEachComp<MeshComponent>(
          [&](MeshComponent& mesh_component) { mesh_component.Draw(); });
    });
  }

  if (selected) {
    ScopeAnnotation annot_render_("Outline");
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    outline_shader_->Use();
    outline_shader_->SetUniform(outline_view_uniform_, window.GetView());
    outline_shader_->SetUniform(outline_projection_uniform_,
                                window.GetProjection());

    selected->ForEachComp<TransformComponent>(
        [&](TransformComponent& transform_component) {
          Eigen::Vector3f s = Eigen::Vector3f::Ones() * 1.05f;
          Eigen::Matrix4f t =
              transform_component.transform * Eigen::Scale(s.x(), s.y(), s.z());
          outline_shader_->SetUniform(outline_model_uniform_, t);
        });

    outline_shader_->SendUniforms();
    selected->ForEachComp<MeshComponent>(
        [&](MeshComponent& mesh_component) { mesh_component.Draw(); });
  }

  OpenGl::BindVertexArray(0);
}