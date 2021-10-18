#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "components/camera_component.hpp"
#include "components/mesh_component.hpp"
#include "components/point_light_component.hpp"
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

struct PointLightUniform {
  UniformHandle location;
  UniformHandle ambient;
  UniformHandle diffuse;
  UniformHandle specular;
};

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

    MaterialUniform material_uniform;
    material_uniform.diffuse = shader->GetUniform("material.diffuse");
    material_uniform.specular = shader->GetUniform("material.specular");
    material_uniform.shininess = shader->GetUniform("material.shininess");

    shader->SetUniform(material_uniform.diffuse, container_diffuse);
    shader->SetUniform(material_uniform.specular, container_specular);
    shader->SetUniform(material_uniform.shininess, 32.0f);

    PointLightUniform light_uniform;
    light_uniform.location = shader->GetUniform("light.location");
    light_uniform.ambient = shader->GetUniform("light.ambient");
    light_uniform.diffuse = shader->GetUniform("light.diffuse");
    light_uniform.specular = shader->GetUniform("light.specular");

    auto model_uniform = shader->GetUniform("model");
    auto view_uniform = shader->GetUniform("view");
    auto view_location_uniform = shader->GetUniform("viewLocation");
    auto projection_uniform = shader->GetUniform("projection");
    auto tex_multiplier_uniform = shader->GetUniform("texCoordMultiplier");

    World world;

    // Create entity with light component
    Entity& point_light = world.SpawnEntity<Entity>();
    {
      const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
      point_light.SetName("PointLight");
      PointLightComponent& light =
          point_light.AddComponent<PointLightComponent>();
      light.diffuse = light_color;
      light.ambient = light_color * 0.1f;
      light.specular = glm::vec3(1.0f, 1.0f, 1.0f);
      MeshComponent& mesh = point_light.AddComponent<MeshComponent>();
      mesh.MakeCube(0.2f, light_color, shader);
      TransformComponent& transform =
          point_light.AddComponent<TransformComponent>();
      transform.transform =
          glm::translate(transform.transform, glm::vec3(1.0f, 1.0f, 1.0f));
    }

    // Create entity with camera component and link it with window
    {
      auto& entity = world.SpawnEntity<Entity>();
      entity.SetName("Camera");
      CameraComponent& camera = entity.AddComponent<CameraComponent>();
      windows.back()->SetCamera(&camera);
      entity.AddComponent<TransformComponent>();
    }

    // Create entity with mesh component
    {
      auto& entity = world.SpawnEntity<Entity>();
      entity.SetName("mesh");
      MeshComponent& mesh = entity.AddComponent<MeshComponent>();
      const glm::vec3 cube_color(1.0f, 1.0f, 1.0f);
      mesh.MakeCube(1.0f, cube_color, shader);  //
      entity.AddComponent<TransformComponent>();
    }

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
        shader->SetUniform(view_uniform, window->GetView());
        shader->SetUniform(view_location_uniform, window->GetCamera()->eye);
        shader->SetUniform(projection_uniform, window->GetProjection());

        point_light.ForEachComp<TransformComponent>(
            [&](TransformComponent& transform_component) {
              auto& tr = transform_component.transform;
              auto l = tr[3];
              // const float a = glm::radians(90.0f) * frame_delta_time;
              const float a = 0.0f;
              l = glm::rotate(l, a, glm::vec3(0.0f, 0.0f, 1.0f));
              tr[3] = l;
              shader->SetUniform(light_uniform.location, glm::vec3(l));
            });

        point_light.ForEachComp<PointLightComponent>(
            [&](PointLightComponent& light_component) {
              shader->SetUniform(light_uniform.diffuse,
                                 light_component.diffuse);
              shader->SetUniform(light_uniform.specular,
                                 light_component.specular);
              shader->SetUniform(light_uniform.ambient,
                                 light_component.ambient);
            });

        world.ForEachEntity([&](Entity& entity) {
          entity.ForEachComp<TransformComponent>(
              [&](TransformComponent& transform_component) {
                shader->SetUniform(model_uniform,
                                   transform_component.transform);
              });

          shader->SendUniforms();
          entity.ForEachComp<MeshComponent>(
              [&](MeshComponent& mesh_component) { mesh_component.Draw(); });

          static int k = 0;
          ++k;
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