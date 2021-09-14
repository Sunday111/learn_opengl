#pragma once

#include <memory>
#include <string>

#include "gl_api.hpp"
#include "include_glm.hpp"

class Shader;

class Vertex {
 public:
  glm::vec3 position;
  glm::vec2 tex_coord;
  glm::vec3 color;
};

class Model3d {
 public:
  Model3d();
  ~Model3d();

  void Create(const std::string& path, GLuint texture,
              const std::shared_ptr<Shader>& shader);

  void Create(const std::span<const Vertex>& vertices,
              const std::span<const ui32>& indices, GLuint texture,
              const std::shared_ptr<Shader>& shader);
  void Draw();

 private:
  size_t num_indices_ = 0;
  std::shared_ptr<Shader> shader_;
  GLuint vao_;  // vertex array object
  GLuint vbo_;  // vertex buffer object
  GLuint ebo_;  // element buffer object
  GLuint texture_ = 0;
};