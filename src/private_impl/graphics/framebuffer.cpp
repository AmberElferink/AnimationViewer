#include "framebuffer.h"

#include <cassert>

#include <glad/glad.h>

#include "texture.h"
#include <glm/vec4.hpp>

using namespace AnimationViewer::Graphics;

std::unique_ptr<Framebuffer>
AnimationViewer::Graphics::Framebuffer::create(const std::unique_ptr<Texture>* textures,
                                               uint8_t size)
{
  uint32_t frameBuffer = 0;
  glGenFramebuffers(1, &frameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

  std::vector<uint32_t> bufs;
  bufs.resize(size);

  for (uint8_t i = 0; i < size; ++i) {
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i]->native_texture, 0);
    bufs[i] = GL_COLOR_ATTACHMENT0 + i;
  }
  glDrawBuffers(static_cast<GLsizei>(bufs.size()), bufs.data());

  return std::unique_ptr<Framebuffer>(new Framebuffer(frameBuffer, size));
}

std::unique_ptr<Framebuffer>
Framebuffer::default_framebuffer()
{
  return std::unique_ptr<Framebuffer>(new Framebuffer(0, 0));
}

Framebuffer::Framebuffer(uint32_t native_handle,
                         uint8_t size)
  : native_handle(native_handle)
  , size(size)
{}

Framebuffer::~Framebuffer()
{
  if (native_handle > 0) {
    glDeleteFramebuffers(1, &native_handle);
  }
}

void
Framebuffer::clear(const std::vector<glm::vec4>& color, const std::vector<float>& depth) const
{
  assert(color.size() == size || native_handle == 0);
  bind();
  for (uint8_t i = 0; i < color.size(); ++i) {
    glClearBufferfv(GL_COLOR, i, reinterpret_cast<const GLfloat*>(&color[i]));
  }
  for (uint8_t i = 0; i < depth.size(); ++i) {
    glClearBufferfv(GL_DEPTH, i, reinterpret_cast<const GLfloat*>(&depth[i]));
  }
}

void
Framebuffer::bind() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, native_handle);
}
