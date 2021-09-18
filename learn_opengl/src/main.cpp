#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "camera.hpp"
#include "debug/gl_debug_messenger.hpp"
#include "image_loader.hpp"
#include "include_glfw.hpp"
#include "include_glm.hpp"
#include "include_imgui.h"
#include "integer.hpp"
#include "model3d.hpp"
#include "properties_widget.hpp"
#include "read_file.hpp"
#include "shader/shader.hpp"
#include "shader/shader_manager.hpp"
#include "template/class_member_traits.hpp"
#include "window.hpp"

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
    const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::unique_ptr<Window>> windows;

    const auto content_dir = exe_file.parent_path() / "content";
    const auto shaders_dir = content_dir / "shaders";
    const auto textures_dir = content_dir / "textures";
    const auto models_dir = content_dir / "models";
    auto shader_manager =
        std::make_unique<ShaderManager>(content_dir / "shaders");

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
    {
      windows.front()->MakeContextCurrent();
      InitializeGLAD();
    }

    GlDebugMessenger::Start();
    OpenGl::EnableDepthTest();

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windows.back()->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ProgramProperties properties;
    ParametersWidget widget(&properties);
    auto shader = shader_manager->LoadShader("simple.shader.json");
    const ui32 color_uniform = shader->GetUniformLocation("globalColor");
    const ui32 tex_mul_uniform =
        shader->GetUniformLocation("texCoordMultiplier");
    const ui32 model_uniform = shader->GetUniformLocation("model");
    const ui32 view_uniform = shader->GetUniformLocation("view");
    const ui32 projection_uniform = shader->GetUniformLocation("projection");
    const GLuint texture = LoadTexture(textures_dir / "viking_room.png");

    std::vector<std::unique_ptr<Model3d>> models;
    models.push_back(std::make_unique<Model3d>());
    models.back()->Create((models_dir / "viking_room.obj").string(), texture,
                          shader);

    UpdateProperties<true>(properties);
    shader->Use();
    OpenGl::SetUniform(color_uniform, properties.Get(properties.global_color));
    OpenGl::SetUniform(tex_mul_uniform, properties.Get(properties.tex_mult));

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

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        properties.MarkAllChanged(false);
        widget.Update();
        // ImGui::ShowDemoWindow();

        UpdateProperties(properties);

        // Rendering
        ImGui::Render();

        OpenGl::Viewport(0, 0, static_cast<GLsizei>(window->GetWidth()),
                         static_cast<GLsizei>(window->GetHeight()));
        OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        properties.OnChange(properties.global_color,
                            [&](const glm::vec4& color) {
                              OpenGl::SetUniform(color_uniform, color);
                            });
        properties.OnChange(properties.tex_mult, [&](const glm::vec2& color) {
          OpenGl::SetUniform(tex_mul_uniform, color);
        });

        OpenGl::SetUniform(view_uniform, window->GetView());
        OpenGl::SetUniform(projection_uniform, window->GetProjection());

        for (auto& model : models) {
          OpenGl::SetUniform(model_uniform, glm::mat4(1.0f));
          model->Draw();
        }
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