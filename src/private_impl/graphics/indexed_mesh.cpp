#include "indexed_mesh.h"

#include <cassert>

#include <glad/glad.h>

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
const std::vector<IndexedMesh::MeshAttributes> full_screen_quad_attributes = {
  IndexedMesh::MeshAttributes{ GL_FLOAT, 2 },
};
} // namespace

std::unique_ptr<IndexedMesh>
IndexedMesh::create(const std::vector<MeshAttributes>& attributes,
                    const void* vertices,
                    uint32_t vertex_size,
                    const uint16_t* indices,
                    uint16_t index_count)
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
    new IndexedMesh(buffers[0], buffers[1], vao, attributes, index_count));
}

std::unique_ptr<IndexedMesh>
IndexedMesh::create_full_screen_quad()
{
  auto fullscreen_quad =
    IndexedMesh::create(full_screen_quad_attributes,
                        full_screen_quad_vertices,
                        sizeof(full_screen_quad_vertices),
                        full_screen_quad_indices,
                        sizeof(full_screen_quad_indices) / sizeof(full_screen_quad_indices[0]));

  return fullscreen_quad;
}

IndexedMesh::IndexedMesh(uint32_t vertex_buffer,
                         uint32_t index_buffer,
                         uint32_t vao,
                         std::vector<MeshAttributes> attributes,
                         uint16_t element_count)

  : vertex_buffer(vertex_buffer)
  , index_buffer(index_buffer)
  , vao(vao)
  , attributes(std::move(attributes))
  , element_count(element_count)
{}

IndexedMesh::~IndexedMesh()
{
  glDeleteBuffers(2, reinterpret_cast<uint32_t*>(this));
  glDeleteVertexArrays(1, &vao);
}

void
IndexedMesh::draw() const
{
  bind();
  glDrawElements(GL_TRIANGLES, element_count, GL_UNSIGNED_SHORT, nullptr);
}

void
IndexedMesh::bind() const
{
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  uint32_t attr_index = 0;
  uintptr_t attr_offset = 0;
  for (auto& attr : attributes) {
    auto size = attr.count;
    switch (attr.type) {
      case GL_FLOAT:
        size *= sizeof(float);
        break;
      default:
        // printf("unsupported type\n");
        assert(false);
        return;
    }
    glEnableVertexAttribArray(attr_index);
    glVertexAttribPointer(attr_index,
                          attr.count,
                          attr.type,
                          GL_FALSE,
                          size,
                          reinterpret_cast<const void*>(attr_offset));
    attr_index++;
    attr_offset += size;
  }
}
