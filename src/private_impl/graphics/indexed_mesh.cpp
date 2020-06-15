#include "indexed_mesh.h"

#include <cassert>
#include <cmath>

#include <glad/glad.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/vec3.hpp>

using namespace AnimationViewer::Graphics;

namespace {
const float full_screen_quad_vertices[8] = {
  // Top left
  0.0f,
  0.0f,
  // Top right
  1.0f,
  0.0f,
  // Bottom left
  1.0f,
  1.0f,
  // Bottom right
  0.0f,
  1.0f,
};
const uint16_t full_screen_quad_indices[6] = { 0, 1, 2, 2, 3, 0 };
const std::vector<IndexedMesh::MeshAttributes> pos_vec_2_attributes = {
  IndexedMesh::MeshAttributes{ GL_FLOAT, 2 },
};
const std::vector<IndexedMesh::MeshAttributes> pos_vec_3_attributes = {
  IndexedMesh::MeshAttributes{ GL_FLOAT, 3 },
};

// 3 floats for position, 3 floats for normals //, 3 floats for tangent, 2 floats uv
const float box_vertices[(3 + 3) * 2 * 3 * 6] = {
  // ,--------------------------------------- x
  // |      ,-------------------------------- y
  // |      |      ,------------------------- z
  // |      |      |     ,------------------- normal x
  // |      |      |     |      ,------------ normal y
  // |      |      |     |      |      ,----- normal z
  // |      |      |     |      |      |
  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, // 0 Front Face
  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, // 1
  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 2

  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 3
  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 4
  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f, // 5

  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, // 6 Back Face
  0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, // 7
  -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, // 8

  -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, // 9
  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, // 10
  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, // 11

  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 12 Top Face
  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, // 13
  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, // 14

  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f, // 15
  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 16
  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 17

  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, // 18 Bottom Face
  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, // 19
  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, // 20

  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, // 21
  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f, // 22
  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f, // 23

  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, // 24 Left Face
  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f, // 25
  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, // 26

  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f, // 27
  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f, // 28
  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f, // 29

  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, // 30 Right Face
  0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f, // 31
  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 32

  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 33
  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f, // 34
  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f, // 35
};
const uint16_t box_indices[36] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                   12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                   24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };
const std::vector<IndexedMesh::MeshAttributes> box_attributes = {
  IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Position
  IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Normal
};

} // namespace

std::unique_ptr<IndexedMesh>
IndexedMesh::create(const std::vector<MeshAttributes>& attributes,
                    const void* vertices,
                    uint32_t vertex_size,
                    const uint16_t* indices,
                    uint16_t index_count,
                    PrimitiveTopology topology)
{
  uint32_t buffers[2];
  uint32_t vao;
  glGenBuffers(2, buffers);
  glGenVertexArrays(1, &vao);

  glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);

  glBufferData(GL_ARRAY_BUFFER, vertex_size, vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(indices[0]), indices, GL_STATIC_DRAW);

  return std::unique_ptr<IndexedMesh>(
    new IndexedMesh(buffers[0], buffers[1], vao, attributes, topology, index_count));
}

std::unique_ptr<IndexedMesh>
IndexedMesh::create_full_screen_quad()
{
  auto fullscreen_quad =
    IndexedMesh::create(pos_vec_2_attributes,
                        full_screen_quad_vertices,
                        sizeof(full_screen_quad_vertices),
                        full_screen_quad_indices,
                        sizeof(full_screen_quad_indices) / sizeof(full_screen_quad_indices[0]),
                        PrimitiveTopology::TriangleList);

  return fullscreen_quad;
}

std::unique_ptr<IndexedMesh>
IndexedMesh::create_disk_3_fan(uint32_t triangle_count, float radius)
{
  std::vector<glm::vec3> vertices(3 * triangle_count + 1);
  vertices[0] = glm::vec3();
  // XZ
  for (uint32_t i = 0; i < triangle_count; ++i) {
    float tau = static_cast<float>(i) / triangle_count * 2.0f * glm::pi<float>();
    vertices[i + 1] = radius * glm::vec3(std::cos(tau), 0.0f, std::sin(tau));
  }
  // XY
  for (uint32_t i = 0; i < triangle_count; ++i) {
    float tau = static_cast<float>(i) / triangle_count * 2.0f * glm::pi<float>();
    vertices[i + triangle_count + 1] = radius * glm::vec3(std::cos(tau), std::sin(tau), 0.0f);
  }
  // YZ
  for (uint32_t i = 0; i < triangle_count; ++i) {
    float tau = static_cast<float>(i) / triangle_count * 2.0f * glm::pi<float>();
    vertices[i + 2 * triangle_count + 1] = radius * glm::vec3(0.0f, -std::cos(tau), std::sin(tau));
  }

  std::vector<uint16_t> indices;
  indices.reserve(3 * (triangle_count + 1));
  indices.push_back(0);
  // XZ plane: 360
  for (uint32_t i = 0; i < triangle_count; ++i) {
    indices.push_back(1 + i);
  }
  indices.push_back(1); // loop back around (XZ)
  // XY plane: 270
  for (uint32_t i = 0; i < (triangle_count * 3) / 4; ++i) {
    indices.push_back(1 + triangle_count + i + 1);
  }
  // YZ plane: 360
  for (uint32_t i = 1; i < triangle_count; ++i) {
    indices.push_back(1 + 2 * triangle_count + i);
  }
  indices.push_back(2 * triangle_count + 1); // loop back around (YZ)
  // XY plane: 90
  for (uint32_t i = (triangle_count * 3) / 4 + 1; i < triangle_count; ++i) {
    indices.push_back(1 + triangle_count + i);
  }
  indices.push_back(triangle_count + 1); // loop back around (XY)

  return IndexedMesh::create(pos_vec_3_attributes,
                             vertices.data(),
                             vertices.size() * sizeof(vertices[0]),
                             indices.data(),
                             indices.size(),
                             PrimitiveTopology::TriangleFan);
}

std::unique_ptr<IndexedMesh>
IndexedMesh::create_box()
{
  auto box = IndexedMesh::create(box_attributes,
                                 box_vertices,
                                 sizeof(box_vertices),
                                 box_indices,
                                 sizeof(box_indices) / sizeof(box_indices[0]),
                                 PrimitiveTopology::TriangleList);

  return box;
}

IndexedMesh::IndexedMesh(uint32_t vertex_buffer,
                         uint32_t index_buffer,
                         uint32_t vao,
                         std::vector<MeshAttributes> attributes,
                         PrimitiveTopology topology,
                         uint16_t element_count)

  : vertex_buffer_(vertex_buffer)
  , index_buffer_(index_buffer)
  , vao_(vao)
  , attributes_(std::move(attributes))
  , topology_(topology)
  , element_count_(element_count)
{}

IndexedMesh::~IndexedMesh()
{
  glDeleteBuffers(2, reinterpret_cast<uint32_t*>(this));
  glDeleteVertexArrays(1, &vao_);
}

void
IndexedMesh::draw() const
{
  bind();
  glDrawElements(static_cast<uint32_t>(topology_), element_count_, GL_UNSIGNED_SHORT, nullptr);
}

void
IndexedMesh::bind() const
{
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

  uint32_t total_stride = 0;
  std::vector<uint32_t> offsets;
  offsets.reserve(attributes_.size());
  uint32_t attr_index = 0;
  for (auto& attr : attributes_) {
    auto size = attr.count;
    switch (attr.type) {
      case GL_FLOAT:
        size *= sizeof(float);
        break;
      case GL_UNSIGNED_INT:
        size *= sizeof(uint32_t);
        break;
      case GL_UNSIGNED_SHORT:
        size *= sizeof(uint16_t);
        break;
      case GL_UNSIGNED_BYTE:
        size *= sizeof(uint8_t);
        break;
      default:
        // printf("unsupported type\n");
        assert(false);
        return;
    }
    offsets.push_back(total_stride);
    total_stride += size;
    attr_index++;
  }

  for (uint32_t i = 0; i < offsets.size(); ++i) {
    glVertexAttribPointer(i,
                          attributes_[i].count,
                          attributes_[i].type,
                          GL_FALSE,
                          total_stride,
                          reinterpret_cast<const void*>(offsets[i]));
    glEnableVertexAttribArray(i);
  }
}
