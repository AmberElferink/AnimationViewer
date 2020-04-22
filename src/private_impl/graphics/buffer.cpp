#include "buffer.h"

#include <cassert>

#include <glad/glad.h>

using AnimationViewer::Graphics::Buffer;

std::unique_ptr<Buffer>
Buffer::create(uint32_t size)
{
  uint32_t buffer = 0;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, buffer);
  glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
  return std::unique_ptr<Buffer>(new Buffer(buffer, size));
}

Buffer::Buffer(uint32_t native_handle, uint32_t size)
  : native_handle(native_handle)
  , size(size)
{}

Buffer::~Buffer()
{
  glDeleteBuffers(1, &native_handle);
}

void
Buffer::set_debug_name([[maybe_unused]] const std::string& name) const
{
#if !__EMSCRIPTEN__
  glObjectLabel(GL_BUFFER, native_handle, -1, name.c_str());
#endif
}

void
Buffer::bind(uint32_t index) const
{
  glBindBuffer(GL_UNIFORM_BUFFER, native_handle);
  glBindBufferBase(GL_UNIFORM_BUFFER, index, native_handle);
}

void
Buffer::upload(const void* data, [[maybe_unused]] uint32_t _size) const
{
  assert(this->size == _size);
  glBindBuffer(GL_UNIFORM_BUFFER, native_handle);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
}
