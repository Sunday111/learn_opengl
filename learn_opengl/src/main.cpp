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
#include "debug/gl_debug_messenger.hpp"
#include "entities/entity.hpp"
#include "image_loader.hpp"
#include "integer.hpp"
#include "name_cache/name_cache.hpp"
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

template <typename Element, typename Default>
void BatchHelper(size_t num_elements, size_t batch_size, size_t first,
                 Element&& handle_element, Default&& handle_default) {
  for (size_t batch_index = 0; batch_index < batch_size; ++batch_index) {
    const size_t index = first + batch_index;
    if (index < num_elements) {
      handle_element(index, batch_index);
    } else {
      handle_default(batch_index);
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

template <typename T, typename F>
void RefreshUniformsArray(std::vector<T>& uniforms, Shader& shader,
                          size_t batch_size, F update_fn) {
  [[unlikely]] if (batch_size != uniforms.size()) {
    uniforms.resize(batch_size);
    for (size_t idx = 0; idx < batch_size; ++idx) {
      uniforms[idx] = update_fn(shader, idx);
    }
    // PrintUniforms(std::span{uniforms});
  }
}

template <typename UniformType, typename LightComponent, typename F>
void BatchLight(
    size_t& next_light, Shader& shader, DefineHandle& def_batch_size,
    std::vector<UniformType>& uniforms,
    std::vector<std::pair<TransformComponent*, LightComponent*>>& lights,
    const std::pair<TransformComponent*, LightComponent*>& default_light,
    F update_fn) {
  const auto batch_size =
      static_cast<size_t>(shader.GetDefineValue<int>(def_batch_size));
  RefreshUniformsArray(uniforms, shader, batch_size, update_fn);

  BatchHelper(
      lights.size(), batch_size, next_light,
      [&](size_t index, size_t batch_index) {
        auto [t, l] = lights[index];
        auto& u = uniforms[batch_index];
        ApplyUniforms(u, shader, *t, *l);
      },
      [&](size_t batch_index) {
        auto [t, l] = default_light;
        auto& u = uniforms[batch_index];
        ApplyUniforms(u, shader, *t, *l);
      });

  next_light += batch_size;
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
    auto shader = std::make_shared<Shader>("simple.shader.json");
    auto container_diffuse =
        texture_manager.GetTexture("container.texture.json");
    auto container_specular =
        texture_manager.GetTexture("container_specular.texture.json");

    auto material_uniform = GetMaterialUniform(*shader);

    shader->SetUniform(material_uniform.diffuse, container_diffuse);
    shader->SetUniform(material_uniform.specular, container_specular);
    shader->SetUniform(material_uniform.shininess, 32.0f);

    auto model_uniform = shader->GetUniform("model");
    auto view_uniform = shader->GetUniform("view");
    auto view_location_uniform = shader->GetUniform("viewLocation");
    auto projection_uniform = shader->GetUniform("projection");
    auto tex_multiplier_uniform = shader->GetUniform("texCoordMultiplier");
    auto def_num_point_lights = shader->GetDefine("cv_num_point_lights");
    auto def_num_directional_lights =
        shader->GetDefine("cv_num_directional_lights");
    auto def_num_spot_lights = shader->GetDefine("cv_num_spot_lights");

    World world;

    std::vector<PointLightUniform> point_light_uniforms;
    std::vector<std::pair<TransformComponent*, PointLightComponent*>>
        point_lights;

    std::vector<DirectionalLightUniform> directional_light_uniforms;
    std::vector<std::pair<TransformComponent*, DirectionalLightComponent*>>
        directional_lights;

    std::vector<SpotLightUniform> spot_light_uniforms;
    std::vector<std::pair<TransformComponent*, SpotLightComponent*>>
        spot_lights;

    // Create entity with light component
    {
      Entity& entity = world.SpawnEntity<Entity>();
      const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
      entity.SetName("PointLight");
      PointLightComponent& light = entity.AddComponent<PointLightComponent>();
      light.diffuse = light_color;
      light.ambient = light_color * 0.1f;
      light.specular = light_color;
      MeshComponent& mesh = entity.AddComponent<MeshComponent>();
      mesh.MakeCube(0.2f, light_color, shader);
      TransformComponent& transform = entity.AddComponent<TransformComponent>();
      transform.transform =
          glm::translate(transform.transform, glm::vec3(1.0f, 1.0f, 1.0f));

      point_lights.push_back({&transform, &light});
    }

    // Create entity with light component
    {
      Entity& entity = world.SpawnEntity<Entity>();
      const glm::vec3 light_color(1.0f, 0.0f, 0.0f);
      entity.SetName("PointLight2");
      PointLightComponent& light = entity.AddComponent<PointLightComponent>();
      light.diffuse = light_color;
      light.ambient = light_color * 0.1f;
      light.specular = light_color;
      MeshComponent& mesh = entity.AddComponent<MeshComponent>();
      mesh.MakeCube(0.2f, light_color, shader);
      TransformComponent& transform = entity.AddComponent<TransformComponent>();
      transform.transform =
          glm::translate(transform.transform, glm::vec3(-1.0f, -1.0f, 1.0f));

      point_lights.push_back({&transform, &light});
    }

    // Create entity with directional light component
    {
      Entity& entity = world.SpawnEntity<Entity>();
      const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
      entity.SetName("DirectionalLight");
      auto& light = entity.AddComponent<DirectionalLightComponent>();
      light.diffuse = light_color;
      light.ambient = light_color * 0.1f;
      light.specular = light_color;

      auto& transform = entity.AddComponent<TransformComponent>();
      transform.transform = glm::yawPitchRoll(
          glm::radians(0.0f), glm::radians(0.0f), glm::radians(0.0f));

      directional_lights.push_back({&transform, &light});
    }

    // Create entity with spot light component
    {
      Entity& entity = world.SpawnEntity<Entity>();
      const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
      entity.SetName("SpotLight");
      auto& light = entity.AddComponent<SpotLightComponent>();
      light.diffuse = light_color;
      light.specular = light_color;
      light.innerAngle = glm::cos(glm::radians(12.5f));
      light.outerAngle = glm::cos(glm::radians(15.0f));

      auto& transform = entity.AddComponent<TransformComponent>();
      transform.transform =
          glm::translate(transform.transform, glm::vec3(0.0f, 0.0, 1.0f));
      transform.transform *= glm::yawPitchRoll(
          glm::radians(0.0f), glm::radians(-90.0f), glm::radians(0.0f));

      spot_lights.push_back({&transform, &light});
    }

    // Create entity with camera component and link it with window
    {
      auto& entity = world.SpawnEntity<Entity>();
      entity.SetName("Camera");
      CameraComponent& camera = entity.AddComponent<CameraComponent>();
      windows.back()->SetCamera(&camera);
      entity.AddComponent<TransformComponent>();
    }

    TransformComponent default_transform;
    PointLightComponent default_point_light;
    DirectionalLightComponent default_directional_light;
    SpotLightComponent default_spot_light;

    {
      default_point_light.ambient = glm::vec3(0.0f);
      default_point_light.diffuse = glm::vec3(0.0f);
      default_point_light.specular = glm::vec3(0.0f);
      default_directional_light.ambient = glm::vec3(0.0f);
      default_directional_light.diffuse = glm::vec3(0.0f);
      default_directional_light.specular = glm::vec3(0.0f);
      default_spot_light.diffuse = glm::vec3(0.0f);
      default_spot_light.specular = glm::vec3(0.0f);
    }

    CreateMeshes(world, shader);

    UpdateProperties<true>(properties);
    shader->Use();

    shader->SetUniform(tex_multiplier_uniform, glm::vec2{1.0f, 1.0f});

    auto prev_frame_time = std::chrono::high_resolution_clock::now();

    while (!windows.empty()) {
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
        window->MakeContextCurrent();

        OpenGl::Viewport(0, 0, static_cast<GLsizei>(window->GetWidth()),
                         static_cast<GLsizei>(window->GetHeight()));
        OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        properties.MarkAllChanged(false);
        widget.Update();

        UpdateProperties(properties);

        static int selected_entity = -1;
        ImGui::Begin("Details");
        ImGui::ListBox(
            "Entities", &selected_entity,
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
        if (selected_entity >= 0) {
          Entity* entity =
              world.GetEntityByIndex(static_cast<size_t>(selected_entity));
          if (entity) {
            entity->DrawDetails();
          }
        }
        ImGui::End();

        // Rendering
        ImGui::Render();

        shader->Use();

        size_t next_point_light = 0;
        BatchLight(next_point_light, *shader, def_num_point_lights,
                   point_light_uniforms, point_lights,
                   {&default_transform, &default_point_light},
                   GetPointLightUniform);

        size_t next_directional_light = 0;
        BatchLight(next_directional_light, *shader, def_num_directional_lights,
                   directional_light_uniforms, directional_lights,
                   {&default_transform, &default_directional_light},
                   GetDirectionalLightUniform);

        size_t next_spot_light = 0;
        BatchLight(next_spot_light, *shader, def_num_spot_lights,
                   spot_light_uniforms, spot_lights,
                   {&default_transform, &default_spot_light},
                   GetSpotLightUniform);

        shader->SetUniform(view_uniform, window->GetView());
        shader->SetUniform(view_location_uniform, window->GetCamera()->eye);
        shader->SetUniform(projection_uniform, window->GetProjection());

        world.ForEachEntity([&](Entity& entity) {
          entity.ForEachComp<TransformComponent>(
              [&](TransformComponent& transform_component) {
                shader->SetUniform(model_uniform,
                                   transform_component.transform);
              });

          shader->SendUniforms();
          entity.ForEachComp<MeshComponent>(
              [&](MeshComponent& mesh_component) { mesh_component.Draw(); });
        });
        OpenGl::BindVertexArray(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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