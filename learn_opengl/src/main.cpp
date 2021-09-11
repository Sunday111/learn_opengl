#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <vector>

#include "include_glfw.hpp"
#include "include_glm.hpp"
#include "include_imgui.h"
#include "integer.hpp"
#include "read_file.hpp"
#include "shader_helper.h"
#include "unused_var.hpp"

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

class Window {
 public:
  Window(ui32 width = 800, ui32 height = 600)
      : id_(MakeWindowId()), width_(width), height_(height) {
    Create();
  }

  ~Window() { Destroy(); }

  void MakeContextCurrent() { glfwMakeContextCurrent(window_); }

  [[nodiscard]] bool ShouldClose() const {
    return glfwWindowShouldClose(window_);
  }

  [[nodiscard]] ui32 GetWidth() const noexcept { return width_; }
  [[nodiscard]] ui32 GetHeight() const noexcept { return height_; }

  void ProcessInput() {
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window_, true);
  }

  void SwapBuffers() { glfwSwapBuffers(window_); }
  GLFWwindow* GetGlfwWindow() const { return window_; }

 private:
  static ui32 MakeWindowId() {
    static ui32 next_id = 0;
    return next_id++;
  }

  static void FrameBufferSizeCallback(GLFWwindow* glfw_window, int width,
                                      int height) {
    void* user_pointer = glfwGetWindowUserPointer(glfw_window);
    if (user_pointer) {
      Window* window = reinterpret_cast<Window*>(user_pointer);
      window->OnResize(width, height);
    }
  }

  void Create() {
    window_ =
        glfwCreateWindow(static_cast<int>(width_), static_cast<int>(height_),
                         "LearnOpenGL", NULL, NULL);

    if (!window_) {
      throw std::runtime_error(fmt::format("Failed to create window"));
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FrameBufferSizeCallback);
    spdlog::info("Created window {:d}", id_);
  }

  void Destroy() {
    if (window_) {
      spdlog::info("Destroyed window {:d}", id_);
      glfwDestroyWindow(window_);
      window_ = nullptr;
    }
  }

  void OnResize(int width, int height) {
    width_ = static_cast<ui32>(width);
    height_ = static_cast<ui32>(height);
  }

 private:
  ui32 id_;
  ui32 width_;
  ui32 height_;
  GLFWwindow* window_ = nullptr;
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
  static int once = InitializeGLAD_impl();
  UnusedVar(once);
}

class Model {
 public:
  void Create() {
    vao_ = OpenGl::GenVertexArray();
    vbo_ = OpenGl::GenBuffer();
    ebo_ = OpenGl::GenBuffer();

    // bind Vertex Array Object
    OpenGl::BindVertexArray(vao_);

    // copy our vertices array in a buffer for OpenGL to use
    OpenGl::BindBuffer(GL_ARRAY_BUFFER, vbo_);
    OpenGl::BufferData(GL_ARRAY_BUFFER, std::span(vertices), GL_STATIC_DRAW);

    // copy index array in a element buffer
    OpenGl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    OpenGl::BufferData(GL_ELEMENT_ARRAY_BUFFER, std::span(indices),
                       GL_STATIC_DRAW);

    // set our vertex attributes pointers
    OpenGl::VertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float),
                                nullptr);
    OpenGl::EnableVertexAttribArray(0);
  }

  void Bind() { OpenGl::BindVertexArray(vao_); }
  void Draw() {
    OpenGl::UseProgram(shader_);
    OpenGl::DrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,
                         nullptr);
  }

  GLuint shader_;
  GLuint vao_;  // vertex array object
  GLuint vbo_;  // vertex buffer object
  GLuint ebo_;  // element buffer object
  std::vector<float> vertices;
  std::vector<ui32> indices;
};

std::vector<std::unique_ptr<Model>> MakeTestModels(GLuint shader) {
  std::vector<std::unique_ptr<Model>> models;

  size_t rows = 1;
  size_t columns = 1;
  float min_coord = -1.0f;
  float max_coord = 1.0f;

  const float size_factor = 0.85f;
  const glm::vec2 coord_zero{min_coord, min_coord};
  const glm::vec2 coord_extent{max_coord - min_coord, max_coord - min_coord};
  const glm::vec2 sector_size_x{coord_extent.x / static_cast<float>(columns),
                                0.0f};
  const glm::vec2 sector_size_y{0.0f,
                                coord_extent.y / static_cast<float>(rows)};
  const glm::vec2 sector_size = (sector_size_x + sector_size_y);
  const glm::vec2 square_size = sector_size * size_factor;

  for (size_t row = 0; row < rows; ++row) {
    for (size_t column = 0; column < columns; ++column) {
      const glm::vec2 offset{sector_size_x * static_cast<float>(column) +
                             sector_size_y * static_cast<float>(row)};

      const glm::vec2 top_left =
          coord_zero + offset + (1.0f - size_factor) * 0.5f * sector_size;
      const glm::vec2 bot_right = top_left + square_size;

      models.push_back(std::make_unique<Model>());
      models.back()->vertices = {
          bot_right.x, top_left.y,  0.0f,  // top right
          bot_right.x, bot_right.y, 0.0f,  // bottom right
          top_left.x,  bot_right.y, 0.0f,  // bottom left
          top_left.x,  top_left.y,  0.0f   // top left
      };
      models.back()->indices = {
          0, 1, 3,  // first triangle
          1, 2, 3   // second triangle
      };
      models.back()->shader_ = shader;
      models.back()->Create();
    }
  }
  return models;
}

GLuint CreateTestUnlitShader(const std::filesystem::path& shaders_dir) {
  ShaderProgramInfo shader_info;
  shader_info.vertex = shaders_dir / "vertex_shader.vert";
  shader_info.fragment = shaders_dir / "fragment_shader.frag";
  return MakeShaderProgram(shader_info);
}

int main(int argc, char** argv) {
  UnusedVar(argc);

  try {
    const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::unique_ptr<Window>> windows;

    const auto content_dir = exe_file.parent_path() / "content";
    const auto shaders_dir = content_dir / "shaders";

    GlfwState glfw_state;
    glfw_state.Initialize();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(0);

    windows.push_back(std::make_unique<Window>());

    // GLAD can be initialized only when glfw has window context
    {
      windows.front()->MakeContextCurrent();
      InitializeGLAD();
    }

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windows.back()->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    const GLuint shader_program = CreateTestUnlitShader(shaders_dir);
    std::vector<std::unique_ptr<Model>> models = MakeTestModels(shader_program);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.2f, 0.3f, 0.3f, 1.0f);

    while (!windows.empty()) {
      for (size_t i = 0; i < windows.size();) {
        auto& window = windows[i];

        [[unlikely]] if (window->ShouldClose()) {
          auto erase_it = windows.begin();
          std::advance(erase_it, i);
          windows.erase(erase_it);
          continue;
        }
        window->ProcessInput();
        window->MakeContextCurrent();

        OpenGl::SetClearColor(clear_color.x, clear_color.y, clear_color.z,
                              clear_color.z);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in
        // ImGui::ShowDemoWindow()! You can browse its code to learn more about
        // Dear ImGui!).
        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End
        // pair to created a named window.
        {
          static float f = 0.0f;
          static int counter = 0;

          ImGui::Begin("Hello, world!");  // Create a window called "Hello,
                                          // world!" and append into it.

          ImGui::Text(
              "This is some useful text.");  // Display some text (you can use a
                                             // format strings too)
          ImGui::Checkbox("Demo Window",
                          &show_demo_window);  // Edit bools storing our window
                                               // open/close state
          ImGui::Checkbox("Another Window", &show_another_window);

          ImGui::SliderFloat(
              "float", &f, 0.0f,
              1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
          ImGui::ColorEdit3(
              "clear color",
              reinterpret_cast<float*>(
                  &clear_color));  // Edit 3 floats representing a color

          if (ImGui::Button(
                  "Button"))  // Buttons return true when clicked (most widgets
                              // return true when edited/activated)
            counter++;
          ImGui::SameLine();
          ImGui::Text("counter = %d", counter);

          ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                      static_cast<double>(1000.0f / ImGui::GetIO().Framerate),
                      static_cast<double>(ImGui::GetIO().Framerate));
          ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
          ImGui::Begin(
              "Another Window",
              &show_another_window);  // Pass a pointer to our bool variable
                                      // (the window will have a closing button
                                      // that will clear the bool when clicked)
          ImGui::Text("Hello from another window!");
          if (ImGui::Button("Close Me")) show_another_window = false;
          ImGui::End();
        }

        // Rendering
        ImGui::Render();

        OpenGl::Viewport(0, 0, static_cast<GLsizei>(window->GetWidth()),
                         static_cast<GLsizei>(window->GetHeight()));
        OpenGl::Clear(GL_COLOR_BUFFER_BIT);

        for (auto& model : models) {
          model->Bind();
          model->Draw();
        }
        OpenGl::BindVertexArray(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window->SwapBuffers();
        ++i;
      }

      glfwPollEvents();
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