#pragma once

#include "components/lights/directional_light_component.hpp"
#include "components/lights/point_light_component.hpp"
#include "components/lights/spot_light_component.hpp"
#include "components/transform_component.hpp"
#include "shader/shader.hpp"

class TextureManager;
class Window;
class World;
class Entity;

struct MaterialUniform {
  UniformHandle diffuse;
  UniformHandle specular;
  UniformHandle shininess;
};

struct PointLightUniform {
  UniformHandle location;
  UniformHandle ambient;
  UniformHandle diffuse;
  UniformHandle specular;
  UniformHandle constant;
  UniformHandle linear;
  UniformHandle quadratic;
};

struct DirectionalLightUniform {
  UniformHandle direction;
  UniformHandle ambient;
  UniformHandle diffuse;
  UniformHandle specular;
};

struct SpotLightUniform {
  UniformHandle location;
  UniformHandle direction;
  UniformHandle diffuse;
  UniformHandle specular;
  UniformHandle innerAngle;
  UniformHandle outerAngle;
  UniformHandle constant;
  UniformHandle linear;
  UniformHandle quadratic;
};

class RenderSystem {
 public:
  RenderSystem(TextureManager& texture_manager);
  ~RenderSystem();

  void ApplyLights();

  void Render(Window& window, World& world, Entity* selected);

  TextureManager* texture_manager_;

  TransformComponent default_transform_;
  PointLightComponent default_point_light_;
  DirectionalLightComponent default_directional_light_;
  SpotLightComponent default_spot_light_;
  DefineHandle def_num_point_lights_;
  DefineHandle def_num_directional_lights_;
  DefineHandle def_num_spot_lights_;

  MaterialUniform material_uniform_;
  UniformHandle model_uniform_;
  UniformHandle view_uniform_;
  UniformHandle view_location_uniform_;
  UniformHandle projection_uniform_;
  UniformHandle tex_multiplier_uniform_;

  UniformHandle outline_model_uniform_;
  UniformHandle outline_view_uniform_;
  UniformHandle outline_projection_uniform_;

  std::shared_ptr<Shader> shader_;
  std::shared_ptr<Shader> outline_shader_;
  std::vector<PointLightUniform> point_light_uniforms_;
  std::vector<std::pair<TransformComponent*, PointLightComponent*>>
      point_lights_;
  std::vector<DirectionalLightUniform> directional_light_uniforms_;
  std::vector<std::pair<TransformComponent*, DirectionalLightComponent*>>
      directional_lights_;
  std::vector<SpotLightUniform> spot_light_uniforms_;
  std::vector<std::pair<TransformComponent*, SpotLightComponent*>> spot_lights_;

  std::shared_ptr<Texture> container_diffuse_;
  std::shared_ptr<Texture> container_specular_;
};
