#include "texture.h"

#include <cassert>

#include <array>

#include <glad/glad.h>

struct TextureFormatLookUp
{
  const uint32_t format;
  const uint32_t internal_format;
  const uint32_t type;
  const uint8_t size;
};

constexpr std::array<TextureFormatLookUp, 40> TextureFormatLookUpTable = { {
  /* r_snorm  */ TextureFormatLookUp{ GL_RED, GL_R8_SNORM, GL_BYTE, 1 },
  /* rg_snorm  */ TextureFormatLookUp{ GL_RG, GL_RG8_SNORM, GL_BYTE, 2 },
  /* rgb_snorm  */
  TextureFormatLookUp{ GL_RGB, GL_RGB8_SNORM, GL_BYTE, 3 },
  /* rgba_snorm */
  TextureFormatLookUp{ GL_RGBA, GL_RGBA8_SNORM, GL_BYTE, 4 },

  /* r8f  */ TextureFormatLookUp{ GL_RED, GL_R8, GL_UNSIGNED_BYTE, 1 },
  /* rg8f  */ TextureFormatLookUp{ GL_RG, GL_RG8, GL_UNSIGNED_BYTE, 2 },
  /* rgb8f  */ TextureFormatLookUp{ GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE, 3 },
  /* rgba8f */ TextureFormatLookUp{ GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE, 4 },

  /* r16f  */ TextureFormatLookUp{ GL_RED, GL_R16F, GL_HALF_FLOAT, 2 },
  /* rg16f  */ TextureFormatLookUp{ GL_RG, GL_RG16F, GL_HALF_FLOAT, 4 },
  /* rgb16f  */ TextureFormatLookUp{ GL_RGB, GL_RGB16F, GL_HALF_FLOAT, 6 },
  /* rgba16f  */ TextureFormatLookUp{ GL_RGBA, GL_RGBA16F, GL_HALF_FLOAT, 8 },

  /* r32f  */ TextureFormatLookUp{ GL_RED, GL_R32F, GL_FLOAT, 4 },
  /* rg32f  */ TextureFormatLookUp{ GL_RG, GL_RG32F, GL_FLOAT, 8 },
  /* rgb32f  */ TextureFormatLookUp{ GL_RGB, GL_RGB32F, GL_FLOAT, 12 },
  /* rgba32f */ TextureFormatLookUp{ GL_RGBA, GL_RGBA32F, GL_FLOAT, 16 },

  /* r8i */ TextureFormatLookUp{ GL_RED_INTEGER, GL_R8I, GL_BYTE, 1 },
  /* rg8i */ TextureFormatLookUp{ GL_RG_INTEGER, GL_RG8I, GL_BYTE, 2 },
  /* rgb8i */ TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB8I, GL_BYTE, 3 },
  /* rgba8i */ TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA8I, GL_BYTE, 4 },

  /* r16i */ TextureFormatLookUp{ GL_RED_INTEGER, GL_R16I, GL_SHORT, 2 },
  /* rg16i */ TextureFormatLookUp{ GL_RG_INTEGER, GL_RG16I, GL_SHORT, 4 },
  /* rgb16i */ TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB16I, GL_SHORT, 6 },
  /* rgba16i */ TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA16I, GL_SHORT, 8 },

  /* r32i */ TextureFormatLookUp{ GL_RED_INTEGER, GL_R32I, GL_INT, 4 },
  /* rg32i */ TextureFormatLookUp{ GL_RG_INTEGER, GL_RG32I, GL_INT, 8 },
  /* rgb32i */ TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB32I, GL_INT, 12 },
  /* rgba32i */ TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA32I, GL_INT, 16 },

  /* r8u */ TextureFormatLookUp{ GL_RED_INTEGER, GL_R8UI, GL_UNSIGNED_BYTE, 1 },
  /* rg8u */
  TextureFormatLookUp{ GL_RG_INTEGER, GL_RG8UI, GL_UNSIGNED_BYTE, 2 },
  /* rgb8u */
  TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB8UI, GL_UNSIGNED_BYTE, 3 },
  /* rgba8u */
  TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA8UI, GL_UNSIGNED_BYTE, 4 },

  /* r16u */
  TextureFormatLookUp{ GL_RED_INTEGER, GL_R16UI, GL_UNSIGNED_SHORT, 2 },
  /* rg16u */
  TextureFormatLookUp{ GL_RG_INTEGER, GL_RG16UI, GL_UNSIGNED_SHORT, 4 },
  /* rgb16u */
  TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB16UI, GL_UNSIGNED_SHORT, 6 },
  /* rgba16u */
  TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA16UI, GL_UNSIGNED_SHORT, 8 },

  /* r32u */
  TextureFormatLookUp{ GL_RED_INTEGER, GL_R32UI, GL_UNSIGNED_INT, 4 },
  /* rg32u */
  TextureFormatLookUp{ GL_RG_INTEGER, GL_RG32UI, GL_UNSIGNED_INT, 8 },
  /* rgb32u */
  TextureFormatLookUp{ GL_RGB_INTEGER, GL_RGB32UI, GL_UNSIGNED_INT, 12 },
  /* rgba32u */
  TextureFormatLookUp{ GL_RGBA_INTEGER, GL_RGBA32UI, GL_UNSIGNED_INT, 16 },
} };

constexpr std::array<uint32_t, 2> TextureFilterLookUpTable = { {
  /* linear  */ GL_LINEAR,
  /* nearest */ GL_NEAREST,
} };

using namespace AnimationViewer::Graphics;

std::unique_ptr<Texture>
AnimationViewer::Graphics::Texture::create(uint32_t width,
                                           uint32_t height,
                                           MipMapFilter filter,
                                           Format format)
{
  uint32_t texture = 0;
  uint32_t sampler = 0;
  glGenTextures(1, &texture);
  glGenSamplers(1, &sampler);

  int32_t gl_filter = TextureFilterLookUpTable[static_cast<uint32_t>(filter)];
  int32_t clamp_to_edge = GL_CLAMP_TO_EDGE;
  glSamplerParameteriv(sampler, GL_TEXTURE_WRAP_S, &clamp_to_edge);
  glSamplerParameteriv(sampler, GL_TEXTURE_WRAP_T, &clamp_to_edge);
  glSamplerParameteriv(sampler, GL_TEXTURE_MIN_FILTER, &gl_filter);
  glSamplerParameteriv(sampler, GL_TEXTURE_MAG_FILTER, &gl_filter);

  auto gl_format = TextureFormatLookUpTable[static_cast<uint32_t>(format)];

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               gl_format.internal_format,
               width,
               height,
               0,
               gl_format.format,
               gl_format.type,
               nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_to_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_to_edge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);

  return std::unique_ptr<Texture>(new Texture(texture, sampler, width, height, format));
}

Texture::Texture(uint32_t native_texture,
                 uint32_t native_sampler,
                 uint32_t width,
                 uint32_t height,
                 Format format)
  : native_texture_(native_texture)
  , native_sampler_(native_sampler)
  , width_(width)
  , height_(height)
  , format_(format)
{}

Texture::~Texture()
{
  glDeleteTextures(1, &native_texture_);
  glDeleteSamplers(1, &native_sampler_);
}

void
Texture::set_debug_name([[maybe_unused]] const std::string& name) const
{
#if !__EMSCRIPTEN__
  glObjectLabel(GL_TEXTURE, native_texture_, -1, (name + " texture").c_str());
  glObjectLabel(GL_SAMPLER, native_sampler_, -1, (name + " sampler").c_str());
#endif
}

void
Texture::bind(uint32_t slot) const
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, native_texture_);
  glBindSampler(slot, native_sampler_);
}

void
Texture::upload(const void* data, [[maybe_unused]] uint32_t size) const
{
  auto gl_format = TextureFormatLookUpTable[static_cast<uint32_t>(format_)];
  assert(size == width_ * height_ * gl_format.size);
  glBindTexture(GL_TEXTURE_2D, native_texture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, gl_format.format, gl_format.type, data);
}

uintptr_t
Texture::get_native_handle() const
{
  return native_texture_;
}
