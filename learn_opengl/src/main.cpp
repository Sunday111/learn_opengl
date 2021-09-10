#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <vector>

#include "integer.hpp"
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
  Window(ui32 width = 800, ui32 height = 600) : width_(width), height_(height) {
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
    spdlog::info("Created window");
  }

  void Destroy() {
    if (window_) {
      spdlog::info("Destroyed window");
      glfwDestroyWindow(window_);
      window_ = nullptr;
    }
  }

  void OnResize(int width, int height) {
    width_ = static_cast<ui32>(width);
    height_ = static_cast<ui32>(height);
  }

 private:
  ui32 width_;
  ui32 height_;
  GLFWwindow* window_ = nullptr;
};

int main() {
  spdlog::set_level(spdlog::level::trace);

  std::vector<std::unique_ptr<Window>> windows;

  try {
    GlfwState glfw_state;
    glfw_state.Initialize();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    windows.push_back(std::make_unique<Window>());

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