#include "components/mesh_component.hpp"

#include <unordered_map>

#include "shader/shader.hpp"
#include "template/class_member_traits.hpp"
#include "template/member_offset.hpp"
#include "template/type_to_gl_type.hpp"
#include "tiny_obj_loader.h"

MeshComponent::MeshComponent() = default;
MeshComponent::~MeshComponent() = default;

namespace std {
template <>
struct hash<tinyobj::index_t> {
  size_t operator()(const tinyobj::index_t& index) const noexcept {
    return (hash<int>()(index.vertex_index) ^
            hash<int>()(index.texcoord_index) ^
            hash<int>()(index.normal_index));
  }
};
}  // namespace std

namespace tinyobj {
bool operator==(const index_t& a, const index_t& b) noexcept {
  return a.vertex_index == b.vertex_index &&
         a.texcoord_index == b.texcoord_index &&
         a.normal_index == b.normal_index;
}
}  // namespace tinyobj

static void LoadModel(const std::string& model_path,
                      std::vector<Vertex>& vertices,
                      std::vector<ui32>& indices) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  [[unlikely]] if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                     model_path.data())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<tinyobj::index_t, ui32> index_remap;
  for (const tinyobj::shape_t& shape : shapes) {
    indices.reserve(indices.size() + shape.mesh.indices.size());
    for (const tinyobj::index_t& model_index : shape.mesh.indices) {
      auto map_iterator = index_remap.find(model_index);
      if (map_iterator == index_remap.end()) {
        Vertex v{};
        const size_t vertex_offset =
            static_cast<size_t>(3 * model_index.vertex_index);
        const size_t normal_offset =
            static_cast<size_t>(3 * model_index.normal_index);

        for (size_t i = 0; i != 3; ++i) {
          v.position[static_cast<int>(i)] = attrib.vertices[vertex_offset + i];
          v.normal[static_cast<int>(i)] = attrib.normals[normal_offset + i];
        }

        const size_t texcoord_offset =
            static_cast<size_t>(2 * model_index.texcoord_index);
        v.tex_coord = {attrib.texcoords[texcoord_offset + 0u],
                       1.0f - attrib.texcoords[texcoord_offset + 1u]};
        v.color = {1.0f, 1.0f, 1.0f};
        const ui32 index = static_cast<ui32>(vertices.size());
        vertices.push_back(v);

        auto [it, inserted] = index_remap.insert({model_index, index});
        map_iterator = it;
      }

      indices.push_back(map_iterator->second);
    }
  }
}

template <auto MemberVariablePtr>
void RegisterAttribute(GLuint location, bool normalized) {
  using MemberTraits = ClassMemberTraits<decltype(MemberVariablePtr)>;
  using GlTypeTraits = TypeToGlType<typename MemberTraits::Member>;
  const size_t vertex_stride = sizeof(typename MemberTraits::Class);
  const size_t member_stride = MemberOffset<MemberVariablePtr>();
  OpenGl::VertexAttribPointer(location, GlTypeTraits::Size, GlTypeTraits::Type,
                              normalized, vertex_stride,
                              reinterpret_cast<void*>(member_stride));
  OpenGl::EnableVertexAttribArray(location);
}

void MeshComponent::Create(const std::span<const Vertex>& vertices,
                           const std::span<const ui32>& indices,
                           const std::shared_ptr<Shader>& shader) {
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
  RegisterAttribute<&Vertex::normal>(location++, false);

  shader_ = shader;
  num_indices_ = indices.size();
}

constexpr std::array<ui32, 6> get_square_indices(bool clockwise) {
  if (clockwise) {
    return {0, 1, 2, 3, 2, 1};
  } else {
    return {2, 1, 0, 1, 2, 3};
  }
}

void MeshComponent::MakeCube(float width, const glm::vec3& color,
                             const std::shared_ptr<Shader>& shader) {
  std::vector<Vertex> vertices;
  std::vector<ui32> indices;
  const float half_width = width / 2.0f;
  constexpr bool clockwise = true;

  /* 0 ---- 1
   * |    / |
   * |  /   |
   * |/     |
   * 2 ---- 3
   */

  using v3f = glm::vec3;
  using v2f = glm::vec2;

  v2f tc[4]{v2f{0.0f, 0.0f}, v2f{0.0f, 1.0f}, v2f{1.0f, 0.0f}, v2f{1.0f, 1.0f}};
  auto make_side_pos = [](size_t index, const v3f& x, const v3f& y) -> v3f {
    auto tx = index % 2 ? x : -x;
    auto ty = index / 2 ? y : -y;
    return tx + ty;
  };
  auto make_side_tex_coord = [&](size_t index) -> v2f {
    // return v2f{index % 2 ? 0.0f : 1.0f, index / 2 ? 1.0f : 0.0f};
    return tc[index];
  };

  constexpr std::array<ui32, 6> side_indices = get_square_indices(clockwise);

  auto add_side = [&](const v3f& x, const v3f& y, const v3f z) {
    Vertex v;
    v.color = color;
    const ui32 side_start = static_cast<ui32>(vertices.size());
    for (size_t i = 0; i < 4u; ++i) {
      v.position = (make_side_pos(i, x, y) + z) * half_width;
      v.tex_coord = make_side_tex_coord(i);
      v.normal = z;
      vertices.push_back(v);
    }

    for (const ui32 index : side_indices) {
      indices.push_back(side_start + index);
    }
  };

  constexpr v3f x(1.0f, 0.0f, 0.0f);
  constexpr v3f y(0.0f, 1.0f, 0.0f);
  constexpr v3f z(0.0f, 0.0f, 1.0f);

  add_side(x, y, z);
  add_side(-z, y, x);
  add_side(-x, y, -z);
  add_side(z, y, -x);
  add_side(x, -z, y);
  add_side(x, z, -y);

  Create(vertices, indices, shader);
}

void MeshComponent::Draw() {
  OpenGl::BindVertexArray(vao_);
  OpenGl::DrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_INT, nullptr);
}

void MeshComponent::DrawDetails() {
  SimpleComponentBase<MeshComponent>::DrawDetails();
  if (shader_) {
    shader_->DrawDetails();
  }
}

void MeshComponent::Create(const std::string& path,
                           const std::shared_ptr<Shader>& shader) {
  std::vector<Vertex> vertices;
  std::vector<ui32> indices;
  LoadModel(path, vertices, indices);
  Create(vertices, indices, shader);
}