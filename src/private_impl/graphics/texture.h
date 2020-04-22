#pragma once

#include <cstdint>

#include <memory>
#include <string>

namespace AnimationViewer::Graphics {
struct Texture
{
  enum class Format
  {
    r_snorm,
    rg_snorm,
    rgb_snorm,
    rgba_snorm,

    r8f,
    rg8f,
    rgb8f,
    rgba8f,

    r16f,
    rg16f,
    rgb16f,
    rgba16f,

    r32f,
    rg32f,
    rgb32f,
    rgba32f,

    r8i,
    rg8i,
    rgb8i,
    rgba8i,

    r16i,
    rg16i,
    rgb16i,
    rgba16i,

    r32i,
    rg32i,
    rgb32i,
    rgba32i,

    r8u,
    rg8u,
    rgb8u,
    rgba8u,

    r16u,
    rg16u,
    rgba16u,
    rgb16u,

    r32u,
    rg32u,
    rgb32u,
    rgba32u,
  };

  enum class MipMapFilter
  {
    linear,
    nearest,
  };

  static std::unique_ptr<Texture> create(uint32_t width,
                                         uint32_t height,
                                         MipMapFilter filter,
                                         Format format);
  virtual ~Texture();

  void set_debug_name(const std::string& name) const;
  void bind(uint32_t slot) const;
  void upload(const void* data, uint32_t size) const;
  uintptr_t get_native_handle() const;

private:
  Texture(uint32_t native_texture,
          uint32_t native_sampler,
          uint32_t width,
          uint32_t height,
          Format format);

  const uint32_t native_texture;
  const uint32_t native_sampler;

  const uint32_t width;
  const uint32_t height;
  const Format format;

  friend struct Framebuffer;
};
} // namespace AnimationViewer::Graphics
