#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <vector>

#include "include_glfw.hpp"
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

    windows.push_back(std::make_unique<Window>());

    // GLAD can be initialized only when glfw has window context
    {
      windows.front()->MakeContextCurrent();
      InitializeGLAD();
    }

    GLuint shader_program;

    {
      ShaderProgramInfo shader_info;
      shader_info.vertex = shaders_dir / "vertex_shader.vert";
      shader_info.fragment = shaders_dir / "fragment_shader.frag";
      shader_program = MakeShaderProgram(shader_info);
    }

    float vertices[] = {
        0.5f,  0.5f,  0.0f,  // top right
        0.5f,  -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f, 0.5f,  0.0f   // top left
    };

    unsigned int indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3   // second triangle
    };

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    GLuint VBO = OpenGl::GenBuffer();
    GLuint EBO = OpenGl::GenBuffer();

    // bind Vertex Array Object
    glBindVertexArray(VAO);

    // copy our vertices array in a buffer for OpenGL to use
    OpenGl::BindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // copy index array in a element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    // set our vertex attributes pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

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

        glViewport(0, 0, static_cast<GLsizei>(window->GetWidth()),
                   static_cast<GLsizei>(window->GetHeight()));
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader_program);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

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