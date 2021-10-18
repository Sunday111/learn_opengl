#pragma once

#include <memory>
#include <string>

#include "components/component.hpp"
#include "gl_api.hpp"
#include "wrap/wrap_glm.hpp"

class Shader;

class Vertex {
 public:
  glm::vec3 position;
  glm::vec2 tex_coord;
  glm::vec3 color;
  glm::vec3 normal;
};

class MeshComponent : public SimpleComponentBase<MeshComponent> {
 public:
  MeshComponent();
  ~MeshComponent();

  void Create(const std::string& path, const std::shared_ptr<Shader>& shader);

  void Create(const std::span<const Vertex>& vertices,
              const std::span<const ui32>& indices,
              const std::shared_ptr<Shader>& shader);

  void MakeCube(float width, const glm::vec3& color,
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

namespace reflection {
template <>
struct TypeReflector<MeshComponent> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "MeshComponent";
    handle->guid = "C3F58B85-406E-4C03-A5AF-4CF736813D57";
    handle.SetBaseClass<Component>();
  }
};
}  // namespace reflection
