#pragma once

#include <memory>

#include "integer.hpp"
#include "wrap/wrap_eigen.hpp"

struct GLFWwindow;
class CameraComponent;

class Window {
 public:
  Window(ui32 width = 800, ui32 height = 600);
  ~Window();

  void MakeContextCurrent();

  [[nodiscard]] bool ShouldClose() const noexcept;
  [[nodiscard]] ui32 GetWidth() const noexcept { return width_; }
  [[nodiscard]] ui32 GetHeight() const noexcept { return height_; }
  [[nodiscard]] GLFWwindow* GetGlfwWindow() const noexcept { return window_; }
  [[nodiscard]] Eigen::Matrix4f GetView() const noexcept;
  [[nodiscard]] Eigen::Matrix4f GetProjection() const noexcept;
  [[nodiscard]] float GetAspect() const noexcept {
    return static_cast<float>(GetWidth()) / static_cast<float>(GetHeight());
  }

  void ProcessInput(float dt);
  void SwapBuffers() noexcept;
  void SetCamera(CameraComponent* camera) { camera_ = camera; }
  CameraComponent* GetCamera() const { return camera_; }

 private:
  static ui32 MakeWindowId();
  static Window* GetWindow(GLFWwindow* glfw_window) noexcept;

  template <auto method, typename... Args>
  static void CallWndMethod(GLFWwindow* glfw_window, Args&&... args) {
    [[likely]] if (Window* window = GetWindow(glfw_window)) {
      (window->*method)(std::forward<Args>(args)...);
    }
  }

  static void FrameBufferSizeCallback(GLFWwindow* glfw_window, int width,
                                      int height);
  static void MouseCallback(GLFWwindow* glfw_window, double x, double y);
  static void MouseButtonCallback(GLFWwindow* glfw_window, int button,
                                  int action, int mods);
  static void MouseScrollCallback(GLFWwindow* glfw_window, double x_offset,
                                  double y_offset);

  void Create();
  void Destroy();
  void OnResize(int width, int height);
  void OnMouseMove(Eigen::Vector2f new_cursor);
  void OnMouseButton(int button, int action, [[maybe_unused]] int mods);
  void OnMouseScroll([[maybe_unused]] float dx, float dy);

 private:
  GLFWwindow* window_ = nullptr;
  CameraComponent* camera_ = nullptr;
  Eigen::Vector2f cursor_;
  ui32 id_;
  ui32 width_;
  ui32 height_;
  bool input_mode_ = false;
};