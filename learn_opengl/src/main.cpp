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
#include "properties_widget.hpp"
#include "read_file.hpp"
#include "reflection/predefined.hpp"
#include "reflection/reflection.hpp"
#include "shader/shader.hpp"
#include "template/class_member_traits.hpp"
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

GLuint LoadTexture(const std::filesystem::path& path) {
  std::string string_path = path.string();
  ImageLoader image(string_path);

  const GLuint texture = OpenGl::GenTexture();
  OpenGl::BindTexture2d(texture);

  size_t lod = 0;
  OpenGl::TexImage2d(GL_TEXTURE_2D, lod, GL_RGB, image.GetWidth(),
                     image.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                     image.GetData().data());
  OpenGl::GenerateMipmap2d();
  return texture;
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

    const ui32 model_uniform = shader->GetUniformLocation("model");
    const ui32 view_uniform = shader->GetUniformLocation("view");
    const ui32 view_location_uniform =
        shader->GetUniformLocation("viewLocation");
    const ui32 projection_uniform = shader->GetUniformLocation("projection");
    const ui32 light_color_uniform = shader->GetUniformLocation("lightColor");
    const ui32 light_location_uniform =
        shader->GetUniformLocation("lightLocation");
    const GLuint texture = LoadTexture(textures_dir / "viking_room.png");

    World world;

    // Create entity with light component
    Entity& point_light = world.SpawnEntity<Entity>();
    {
      const glm::vec3 light_color(1.0f, 1.0f, 1.0f);
      point_light.SetName("PointLight");
      PointLightComponent& light =
          point_light.AddComponent<PointLightComponent>();
      light.color = light_color;
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
      // mesh.Create((models_dir / "viking_room.obj").string(), texture,
      // shader);
      const glm::vec3 cube_color(1.0f, 1.0f, 1.0f);
      mesh.MakeCube(1.0f, cube_color, shader);
      entity.AddComponent<TransformComponent>();
    }

    UpdateProperties<true>(properties);
    shader->Use();

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

        ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();

        OpenGl::SetUniform(view_uniform, window->GetView());
        OpenGl::SetUniform(view_location_uniform, window->GetCamera()->eye);
        OpenGl::SetUniform(projection_uniform, window->GetProjection());

        point_light.ForEachComp<TransformComponent>(
            [&](TransformComponent& transform_component) {
              glm::vec3 t(transform_component.transform[3]);
              OpenGl::SetUniform(light_location_uniform, t);
            });

        point_light.ForEachComp<PointLightComponent>(
            [&](PointLightComponent& light_component) {
              OpenGl::SetUniform(light_color_uniform, light_component.color);
            });

        world.ForEachEntity([&](Entity& entity) {
          entity.ForEachComp<TransformComponent>(
              [&](TransformComponent& transform_component) {
                OpenGl::SetUniform(model_uniform,
                                   transform_component.transform);
                OpenGl::SetUniform(model_uniform,
                                   transform_component.transform);
              });
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