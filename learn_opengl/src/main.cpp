#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "image_loader.hpp"
#include "include_glfw.hpp"
#include "include_glm.hpp"
#include "include_imgui.h"
#include "integer.hpp"
#include "properties_widget.hpp"
#include "read_file.hpp"
#include "shader_helper.h"
#include "template/class_member_traits.hpp"
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

class Vertex {
 public:
  glm::vec3 position;
  glm::vec2 tex_coord;
  glm::vec3 color;
};

template <typename T>
struct TypeToGlType;

template <>
struct TypeToGlType<float> {
  static constexpr size_t Size = 1;
  static constexpr GLenum Type = GL_FLOAT;
};

template <typename T, int N>
struct TypeToGlType<glm::vec<N, T>> {
  static constexpr size_t Size = static_cast<size_t>(N);
  static constexpr GLenum Type = TypeToGlType<T>::Type;
};

template <auto Member>
[[nodiscard]] size_t MemberOffset() noexcept {
  using MemberTraits = ClassMemberTraits<decltype(Member)>;
  using T = typename MemberTraits::Class;
  auto ptr = &(reinterpret_cast<T const volatile*>(NULL)->*Member);
  return reinterpret_cast<size_t>(ptr);
}

class Model {
 public:
  void Create(const std::span<const Vertex>& vertices,
              const std::span<const ui32>& indices, GLuint texture) {
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

    GLuint location = 0;
    RegisterAttribute<&Vertex::position>(location++, false);
    RegisterAttribute<&Vertex::tex_coord>(location++, false);
    RegisterAttribute<&Vertex::color>(location++, false);

    texture_ = texture;
    num_indices_ = indices.size();
  }

  template <auto MemberVariablePtr>
  void RegisterAttribute(GLuint location, bool normalized) {
    using MemberTraits = ClassMemberTraits<decltype(MemberVariablePtr)>;
    using GlTypeTraits = TypeToGlType<typename MemberTraits::Member>;
    const size_t vertex_stride = sizeof(typename MemberTraits::Class);
    const size_t member_stride = MemberOffset<MemberVariablePtr>();
    OpenGl::VertexAttribPointer(location, GlTypeTraits::Size,
                                GlTypeTraits::Type, normalized, vertex_stride,
                                reinterpret_cast<void*>(member_stride));
    OpenGl::EnableVertexAttribArray(location);
  }

  void Draw() {
    OpenGl::UseProgram(shader_);
    OpenGl::BindVertexArray(vao_);
    OpenGl::BindTexture2d(texture_);
    OpenGl::DrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_INT, nullptr);
  }

  size_t num_indices_ = 0;
  GLuint shader_;
  GLuint vao_;  // vertex array object
  GLuint vbo_;  // vertex buffer object
  GLuint ebo_;  // element buffer object
  GLuint texture_ = 0;
};

std::vector<std::unique_ptr<Model>> MakeTestModels(GLuint shader,
                                                   GLuint texture) {
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

      std::array<Vertex, 4> v;

      // top right
      v[0].position = {bot_right.x, top_left.y, 0.0f};
      v[0].color = {1.0f, 1.0f, 1.0f};
      v[0].tex_coord = {1.0f, 1.0f};

      // bottom right
      v[1].position = {bot_right.x, bot_right.y, 0.0f};
      v[1].color = {1.0f, 1.0f, 1.0f};
      v[1].tex_coord = {1.0f, 0.0f};

      // bottom left
      v[2].position = {top_left.x, bot_right.y, 0.0f};
      v[2].color = {1.0f, 1.0f, 1.0f};
      v[2].tex_coord = {0.0f, 0.0f};

      // top left
      v[3].position = {top_left.x, top_left.y, 0.0f};
      v[3].color = {1.0f, 1.0f, 1.0f};
      v[3].tex_coord = {0.0f, 1.0f};

      std::array<ui32, 6> indices = {
          0, 1, 3,  // first triangle
          1, 2, 3   // second triangle
      };

      models.push_back(std::make_unique<Model>());
      models.back()->shader_ = shader;
      models.back()->Create(v, indices, texture);
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
void UpdateProperties(const ParametersWidget& w) noexcept {
  auto check_prop = [&](auto index, auto fn) {
    w.CheckPropertyChange<force>(index, fn);
  };

  check_prop(w.GetPolygonModeIndex(), OpenGl::PolygonMode);
  check_prop(w.GetPointSizeIndex(), OpenGl::PointSize);
  check_prop(w.GetLineWidthIndex(), OpenGl::LineWidth);
  check_prop(w.GetBorderColorIndex(), OpenGl::SetTexture2dBorderColor);
  check_prop(w.GetClearColorIndex(),
             [](auto clear_color) { OpenGl::SetClearColor(clear_color); });
  check_prop(w.GetTextureWrapModeSIndex(), [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::S, mode);
  });
  check_prop(w.GetTextureWrapModeTIndex(), [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::T, mode);
  });
  check_prop(w.GetTextureWrapModeRIndex(), [](auto mode) {
    OpenGl::SetTexture2dWrap(GlTextureWrap::R, mode);
  });
}

int main(int argc, char** argv) {
  UnusedVar(argc);

  try {
    const std::filesystem::path exe_file = std::filesystem::path(argv[0]);
    spdlog::set_level(spdlog::level::trace);
    std::vector<std::unique_ptr<Window>> windows;

    const auto content_dir = exe_file.parent_path() / "content";
    const auto shaders_dir = content_dir / "shaders";
    const auto textures_dir = content_dir / "textures";

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

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windows.back()->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ParametersWidget parameters;
    const GLuint shader_program = CreateTestUnlitShader(shaders_dir);

    const ui32 color_uniform =
        OpenGl::GetUniformLocation(shader_program, "globalColor");
    const ui32 tex_mul_uniform =
        OpenGl::GetUniformLocation(shader_program, "texCoordMultiplier");
    const GLuint texture = LoadTexture(textures_dir / "wall.jpg");
    std::vector<std::unique_ptr<Model>> models =
        MakeTestModels(shader_program, texture);

    UpdateProperties<true>(parameters);
    OpenGl::UseProgram(shader_program);
    OpenGl::SetUniform(color_uniform, parameters.GetGlobalColor());
    OpenGl::SetUniform(tex_mul_uniform, parameters.GetTexCoordMultiplier());

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

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        parameters.Update();
        // ImGui::ShowDemoWindow();

        UpdateProperties(parameters);

        // Rendering
        ImGui::Render();

        OpenGl::Viewport(0, 0, static_cast<GLsizei>(window->GetWidth()),
                         static_cast<GLsizei>(window->GetHeight()));
        OpenGl::Clear(GL_COLOR_BUFFER_BIT);
        parameters.CheckPropertyChange(
            parameters.GetGlobalColorIndex(), [&](const glm::vec4& color) {
              OpenGl::SetUniform(color_uniform, color);
            });

        parameters.CheckPropertyChange(parameters.GetTexCoordMultiplierIndex(),
                                       [&](const glm::vec2& color) {
                                         OpenGl::SetUniform(tex_mul_uniform,
                                                            color);
                                       });
        OpenGl::SetTexture2dMinFilter(GlTextureFilter::LinearMipmapLinear);
        OpenGl::SetTexture2dMagFilter(GlTextureFilter::Linear);
        for (auto& model : models) {
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