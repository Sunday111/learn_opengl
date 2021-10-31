#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "components/camera_component.hpp"
#include "components/lights/directional_light_component.hpp"
#include "components/lights/point_light_component.hpp"
#include "components/lights/spot_light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/transform_component.hpp"
#include "entities/entity.hpp"
#include "image_loader.hpp"
#include "integer.hpp"
#include "name_cache/name_cache.hpp"
#include "opengl/debug/annotations.hpp"
#include "opengl/debug/gl_debug_messenger.hpp"
#include "properties_widget.hpp"
#include "read_file.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/predefined.hpp"
#include "reflection/reflection.hpp"
#include "shader/shader.hpp"
#include "template/class_member_traits.hpp"
#include "texture/texture.hpp"
#include "texture/texture_manager.hpp"
#include "window.hpp"
#include "world.hpp"
#include "wrap/wrap_glfw.hpp"
#include "wrap/wrap_glm.hpp"
#include "wrap/wrap_imgui.h"

class GlfwState {
 public:
  ~GlfwState() { Uninitialize(); }

  void Initialize() {
    [[unlikely]] if (!glfwInit()) {
      throw std::runtime_error("failed to initialize glfw");
    }

    spdlog::info("GLFW initialized");
    initialized_ = true;
  }

  void Uninitialize() {
    if (initialized_) {
      glfwTerminate();
      initialized_ = false;
      spdlog::info("GLFW terminated");
    }
  }

 private:
  bool initialized_ = false;
};

int InitializeGLAD_impl() {
  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    throw std::runtime_error("Failed to initialize GLAD");
  }

  return 42;
}

void InitializeGLAD() {
  [[maybe_unused]] static int once = InitializeGLAD_impl();
}

template <bool force = false>
void UpdateProperties(const ProgramProperties& p) noexcept {
  auto check_prop = [&](auto index, auto fn) { p.OnChange<force>(index, fn); };

  check_prop(p.polygon_mode, OpenGl::PolygonMode);
  check_prop(p.point_size, OpenGl::PointSize);
  check_prop(p.line_width, OpenGl::LineWidth);
  check_prop(p.tex_border_color, OpenGl::SetTexture2dBorderColor);
  check_prop(p.clear_color,
             [](auto clear_color) { OpenGl::SetClearColor(clear_color); });
  check_prop(p.wrap_mode_s, [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::S, mode);
  });
  check_prop(p.wrap_mode_t, [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::T, mode);
  });
  check_prop(p.wrap_mode_r, [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::R, mode);
  });

  check_prop(p.min_filter, OpenGl::SetTexture2dMinFilter);
  check_prop(p.mag_filter, OpenGl::SetTexture2dMagFilter);
}

struct MaterialUniform {
  UniformHandle diffuse;
  UniformHandle specular;
  UniformHandle shininess;
};

auto GetMaterialUniform(Shader& s) {
  MaterialUniform u;
  u.diffuse = s.GetUniform("material.diffuse");
  u.specular = s.GetUniform("material.specular");
  u.shininess = s.GetUniform("material.shininess");
  return u;
}

struct PointLightUniform {
  UniformHandle location;
  UniformHandle ambient;
  UniformHandle diffuse;
  UniformHandle specular;
  UniformHandle constant;
  UniformHandle linear;
  UniformHandle quadratic;
};

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

struct DirectionalLightUniform {
  UniformHandle direction;
  UniformHandle ambient;
  UniformHandle diffuse;
  UniformHandle specular;
};

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

void ApplyUniforms(PointLightUniform& u, Shader& s,
                   TransformComponent& transform, PointLightComponent& light) {
  {
    auto l = transform.transform[3];
    s.SetUniform(u.location, glm::vec3(l));
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
    auto& tr = transform.transform;
    glm::vec3 dir(0.0f, 0.0f, -1.0f);
    s.SetUniform(u.location, glm::vec3(tr[3]));
    s.SetUniform(u.direction, dir * glm::mat3(tr));
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
    auto& tr = transform.transform;
    glm::vec3 dir(1.0f, 0.0f, 0.0f);
    dir = dir * glm::mat3(tr);
    s.SetUniform(u.direction, dir);
  }

  s.SetUniform(u.ambient, light.ambient);
  s.SetUniform(u.diffuse, light.diffuse);
  s.SetUniform(u.specular, light.specular);
}

void CreateMeshes(World& world, const std::shared_ptr<Shader>& shader) {
  constexpr float width = 15.0f;
  constexpr float height = 15.0f;
  constexpr size_t nx = 10;
  constexpr size_t ny = 10;

  // Create entity with mesh component
  for (size_t x = 0; x < nx; ++x) {
    for (size_t y = 0; y < ny; ++y) {
      auto& entity = world.SpawnEntity<Entity>();
      entity.SetName(fmt::format("mesh [x:{}, y:{}]", x, y));
      MeshComponent& mesh = entity.AddComponent<MeshComponent>();
      const glm::vec3 cube_color(1.0f, 1.0f, 1.0f);
      mesh.MakeCube(1.0f, cube_color, shader);  //
      auto& t = entity.AddComponent<TransformComponent>();

      auto position = glm::vec3((x * width / nx) - (width / 2),
                                (y * height / ny) - (height / 2), 0.0f);
      t.transform = glm::translate(t.transform, position);
    }
  }
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

class RenderSystem {
 public:
  RenderSystem(TextureManager& texture_manager)
      : texture_manager_(&texture_manager) {
    shader_ = std::make_shared<Shader>("simple.shader.json");
    outline_shader_ = std::make_shared<Shader>("outline.shader.json");
    shader_->Use();

    container_diffuse_ = texture_manager.GetTexture("container.texture.json");
    container_specular_ =
        texture_manager.GetTexture("container_specular.texture.json");

    default_point_light_.ambient = glm::vec3(0.0f);
    default_point_light_.diffuse = glm::vec3(0.0f);
    default_point_light_.specular = glm::vec3(0.0f);
    default_directional_light_.ambient = glm::vec3(0.0f);
    default_directional_light_.diffuse = glm::vec3(0.0f);
    default_directional_light_.specular = glm::vec3(0.0f);
    default_spot_light_.diffuse = glm::vec3(0.0f);
    default_spot_light_.specular = glm::vec3(0.0f);
    def_num_point_lights_ = shader_->GetDefine("cv_num_point_lights");
    def_num_directional_lights_ =
        shader_->GetDefine("cv_num_directional_lights");
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
    shader_->SetUniform(tex_multiplier_uniform_, glm::vec2{1.0f, 1.0f});
  }

  void ApplyLights() {
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

  void Render(Window& window, World& world, Entity* selected) {
    OpenGl::Viewport(0, 0, static_cast<GLsizei>(window.GetWidth()),
                     static_cast<GLsizei>(window.GetHeight()));

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
      world.ForEachEntity([&](Entity& entity) {
        const bool is_selected = (&entity == selected);
        // don't update stencil buffer for not selected objects
        glStencilMask(is_selected ? 0xFF : 0x00);

        entity.ForEachComp<TransformComponent>(
            [&](TransformComponent& transform_component) {
              shader_->SetUniform(model_uniform_,
                                  transform_component.transform);
            });

        shader_->SendUniforms();
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
            auto scaled = glm::scale(transform_component.transform,
                                     glm::vec3(1.1f, 1.1f, 1.1f));
            outline_shader_->SetUniform(outline_model_uniform_, scaled);
          });

      outline_shader_->SendUniforms();
      selected->ForEachComp<MeshComponent>(
          [&](MeshComponent& mesh_component) { mesh_component.Draw(); });
    }

    OpenGl::BindVertexArray(0);
  }

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

void CreatePointLights(World& world, RenderSystem& render_system) {
  size_t num_lights = 14;
  float radius = 5.0f;

  std::array light_colors{
      glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 1.0f, 1.0f)};

  for (size_t light_index = 0; light_index < num_lights; ++light_index) {
    Entity& entity = world.SpawnEntity<Entity>();
    entity.SetName(fmt::format("point lights {}", light_index));
    PointLightComponent& light = entity.AddComponent<PointLightComponent>();
    const auto light_color = light_colors[light_index % light_colors.size()];
    light.diffuse = light_color;
    light.ambient = glm::vec3(0.0f);
    light.specular = light_color;
    MeshComponent& mesh = entity.AddComponent<MeshComponent>();
    mesh.MakeCube(0.2f, light_color, render_system.shader_);
    TransformComponent& transform = entity.AddComponent<TransformComponent>();

    float angle = (360.0f * light_index) / num_lights;
    float y = glm::sin(glm::radians(angle));
    float x = glm::cos(glm::radians(angle));

    const glm::vec3 location =
        glm::vec3(0.0f, 0.0f, 1.0f) + glm::vec3(x, y, 0.0f) * radius;
    transform.transform = glm::translate(transform.transform, location);

    render_system.point_lights_.push_back({&transform, &light});
  }
}

int main([[maybe_unused]] int argc, char** argv) {
  try {
    spdlog::set_level(spdlog::level::warn);
    const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
    std::vector<std::unique_ptr<Window>> windows;

    const auto content_dir = exe_file.parent_path() / "content";
    const auto shaders_dir = content_dir / "shaders";
    const auto textures_dir = content_dir / "textures";
    const auto models_dir = content_dir / "models";
    Shader::shaders_dir_ = content_dir / "shaders";

    GlfwState glfw_state;
    glfw_state.Initialize();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    windows.push_back(std::make_unique<Window>());

    // GLAD can be initialized only when glfw has window context
    windows.front()->MakeContextCurrent();
    InitializeGLAD();

    TextureManager texture_manager(textures_dir);

    GlDebugMessenger::Start();
    OpenGl::EnableDepthTest();

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windows.back()->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ProgramProperties properties;
    ParametersWidget widget(&properties);

    World world;

    RenderSystem render_system(texture_manager);

    //// Create entity with directional light component
    //{
    //  Entity& entity = world.SpawnEntity<Entity>();
    //  const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
    //  entity.SetName("DirectionalLight");
    //  auto& light = entity.AddComponent<DirectionalLightComponent>();
    //  light.diffuse = light_color;
    //  light.ambient = light_color * 0.1f;
    //  light.specular = light_color;
    //
    //  auto& transform = entity.AddComponent<TransformComponent>();
    //  transform.transform = glm::yawPitchRoll(
    //      glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f));
    //
    //  render_system.directional_lights_.push_back({&transform, &light});
    //}
    //
    //// Create entity with spot light component
    //{
    //  Entity& entity = world.SpawnEntity<Entity>();
    //  const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
    //  entity.SetName("SpotLight");
    //  auto& light = entity.AddComponent<SpotLightComponent>();
    //  light.diffuse = light_color;
    //  light.specular = light_color;
    //  light.innerAngle = glm::cos(glm::radians(12.5f));
    //  light.outerAngle = glm::cos(glm::radians(15.0f));
    //
    //  auto& transform = entity.AddComponent<TransformComponent>();
    //  transform.transform =
    //      glm::translate(transform.transform, glm::vec3(0.0f, 0.0, 1.0f));
    //  transform.transform *= glm::yawPitchRoll(
    //      glm::radians(0.0f), glm::radians(-90.0f), glm::radians(0.0f));
    //
    //  render_system.spot_lights_.push_back({&transform, &light});
    //}

    // Create entity with camera component and link it with window
    {
      auto& entity = world.SpawnEntity<Entity>();
      entity.SetName("Camera");
      CameraComponent& camera = entity.AddComponent<CameraComponent>();
      windows.back()->SetCamera(&camera);
      entity.AddComponent<TransformComponent>();
    }

    CreateMeshes(world, render_system.shader_);
    CreatePointLights(world, render_system);

    UpdateProperties<true>(properties);

    auto prev_frame_time = std::chrono::high_resolution_clock::now();

    while (!windows.empty()) {
      ScopeAnnotation frame_annotation("Frame");
      const auto current_frame_time = std::chrono::high_resolution_clock::now();
      const auto frame_delta_time =
          std::chrono::duration<float, std::chrono::seconds::period>(
              current_frame_time - prev_frame_time)
              .count();

      for (size_t i = 0; i < windows.size();) {
        auto& window = windows[i];

        [[unlikely]] if (window->ShouldClose()) {
          auto erase_it = windows.begin();
          std::advance(erase_it, i);
          windows.erase(erase_it);
          continue;
        }
        window->ProcessInput(frame_delta_time);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        properties.MarkAllChanged(false);
        widget.Update();

        UpdateProperties(properties);

        static int selected_entity_id = -1;
        Entity* selected_entity = nullptr;
        ImGui::Begin("Details");
        ImGui::ListBox(
            "Entities", &selected_entity_id,
            [](void* data, int idx, const char** name) {
              [[likely]] if (data) {
                World* world = reinterpret_cast<World*>(data);
                Entity* entity =
                    world->GetEntityByIndex(static_cast<size_t>(idx));
                [[likely]] if (entity) {
                  *name = entity->GetName().data();
                  return true;
                }
              }

              return false;
            },
            &world, static_cast<int>(world.GetNumEntities()));
        if (selected_entity_id >= 0) {
          selected_entity =
              world.GetEntityByIndex(static_cast<size_t>(selected_entity_id));
          if (selected_entity) {
            selected_entity->DrawDetails();
          }
        }
        ImGui::End();

        render_system.Render(*window, world, selected_entity);

        {
          ScopeAnnotation imgui_render("ImGUI");
          ImGui::Render();
          ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        window->SwapBuffers();
        ++i;
      }

      glfwPollEvents();
      prev_frame_time = current_frame_time;
    }
  } catch (const std::exception& e) {
    spdlog::critical("Unhandled exception: {}", e.what());
    return -1;
  } catch (...) {
    spdlog::critical("Unknown unhandled exception");
    return -1;
  }

  return 0;
}