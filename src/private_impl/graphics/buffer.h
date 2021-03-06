#pragma once

#include <cstdint>

#include <memory>
#include <string>

namespace AnimationViewer::Graphics {
class Buffer
{
public:
  static std::unique_ptr<Buffer> create(uint32_t size);
  virtual ~Buffer();

  void set_debug_name(const std::string& name) const;
  void bind(uint32_t index) const;
  void upload(const void* data, uint32_t size) const;

protected:
  Buffer(uint32_t size, uint32_t native_handle);

private:
  const uint32_t native_handle_;
  const uint32_t size_;
};
} // namespace AnimationViewer::Graphics
