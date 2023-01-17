#pragma once

#include <memory>
#include <string>

#include "components/component.hpp"
#include "opengl/gl_api.hpp"
#include "reflection/eigen_reflect.hpp"

class Shader;

class Vertex {
 public:
  Eigen::Vector3f position;
  Eigen::Vector2f tex_coord;
  Eigen::Vector3f color;
  Eigen::Vector3f normal;
};

class MeshComponent : public SimpleComponentBase<MeshComponent> {
 public:
  MeshComponent();
  ~MeshComponent();

  void Create(const std::string& path, const std::shared_ptr<Shader>& shader);

  void Create(const std::span<const Vertex>& vertices,
              const std::span<const ui32>& indices,
              const std::shared_ptr<Shader>& shader);

  void MakeCube(float width, const Eigen::Vector3f& color,
                const std::shared_ptr<Shader>& shader);

  void Draw();

  virtual void DrawDetails() override;

 private:
  size_t num_indices_ = 0;
  std::shared_ptr<Shader> shader_;
  GLuint vao_;  // vertex array object
  GLuint vbo_;  // vertex buffer object
  GLuint ebo_;  // element buffer object
};

namespace cppreflection {
template <>
struct TypeReflectionProvider<MeshComponent> {
  [[nodiscard]] inline constexpr static auto ReflectType() {
    return StaticClassTypeInfo<MeshComponent>(
               "MeshComponent",
               edt::GUID::Create("C3F58B85-406E-4C03-A5AF-4CF736813D57"))
        .Base<Component>();
  }
};
}  // namespace cppreflection