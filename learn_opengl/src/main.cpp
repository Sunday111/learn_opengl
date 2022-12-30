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
#include "integer.hpp"
#include "name_cache/name_cache.hpp"
#include "opengl/debug/annotations.hpp"
#include "opengl/debug/gl_debug_messenger.hpp"
#include "properties_widget.hpp"
#include "read_file.hpp"
#include "reflection/glm_reflect.hpp"
#include "reflection/register_types.hpp"
#include "render_system.hpp"
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

void Main([[maybe_unused]] int argc, char** argv) {
  spdlog::set_level(spdlog::level::warn);
  const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
  RegisterReflectionTypes();
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

  {
    ui32 window_width = 800;
    ui32 window_height = 800;

    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
      float x_scale, y_scale;
      glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
      window_width = static_cast<ui32>(window_width * x_scale);
      window_height = static_cast<ui32>(window_height * y_scale);
    }

    windows.push_back(std::make_unique<Window>(window_width, window_height));
  }

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

  if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
    float xscale, yscale;
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);

    ImGui::GetStyle().ScaleAllSizes(2);
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig font_config{};
    font_config.SizePixels = 13 * xscale;
    io.Fonts->AddFontDefault(&font_config);
  }

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
}

int main(int argc, char** argv) {
  try {
    Main(argc, argv);
  } catch (const std::exception& e) {
    spdlog::critical("Unhandled exception: {}", e.what());
    return -1;
  } catch (...) {
    spdlog::critical("Unknown unhandled exception");
    return -1;
  }

  return 0;
}