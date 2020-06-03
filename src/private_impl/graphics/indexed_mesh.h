#pragma once

#include <cstdint>

#include <memory>
#include <vector>

namespace AnimationViewer::Graphics {
struct IndexedMesh
{
  struct MeshAttributes
  {
    uint32_t type;
    uint32_t count;
  };
  const uint32_t vertex_buffer_;
  const uint32_t index_buffer_;
  const uint32_t vao_;
  const std::vector<MeshAttributes> attributes_;
  const uint16_t element_count_;

  static std::unique_ptr<IndexedMesh> create(const std::vector<MeshAttributes>& attributes,
                                             const void* vertices,
                                             uint32_t vertex_size,
                                             const uint16_t* indices,
                                             uint16_t index_count);
  static std::unique_ptr<IndexedMesh> create_full_screen_quad();
  static std::unique_ptr<IndexedMesh> create_box();

  virtual ~IndexedMesh();
  void draw() const;
  void bind() const;

private:
  IndexedMesh(uint32_t vertex_buffer,
              uint32_t index_buffer,
              uint32_t vao,
              std::vector<MeshAttributes> attributes,
              uint16_t element_count);
};
} // namespace AnimationViewer::Graphics
