#pragma once

#include <cstdint>

#include <glm/vec4.hpp>
#include <memory>
#include <vector>

namespace AnimationViewer::Graphics {
struct Texture;

struct Framebuffer
{
  static std::unique_ptr<Framebuffer> create(const std::unique_ptr<Texture>* textures,
                                             uint8_t size);
  static std::unique_ptr<Framebuffer> default_framebuffer();
  virtual ~Framebuffer();

  void bind() const;
  void clear(const std::vector<glm::vec4>& color, const std::vector<float>& depth) const;

private:
  Framebuffer(uint32_t native_handle, uint8_t size);

  const uint32_t native_handle_;
  const uint8_t size_;
};
} // namespace AnimationViewer::Graphics
