#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace AnimationViewer::Graphics {
/// This Abstract base class contains anything which would represent a render
/// state on the GPU
///
/// The purpose of using this class rather than directly setting states is to
/// easily turn on and off states in a predictable and clean way without halting
/// rendering.
class Pipeline
{
public:
  enum class Type
  {
    RasterOpenGL,
  };
  struct CreateInfo
  {
    const uint32_t* vertex_shader_binary;
    uint32_t vertex_shader_size;
    std::string vertex_shader_entry_point;
    const uint32_t* fragment_shader_binary;
    uint32_t fragment_shader_size;
    std::string fragment_shader_entry_point;
  };
  /// Factory function from which all types of pipelines can be created
  static std::unique_ptr<Pipeline> create(Type type, const CreateInfo& info);
  virtual ~Pipeline() = default;
  virtual void bind() = 0;

  virtual uint32_t get_native_handle() const = 0;
};
} // namespace AnimationViewer::Graphics
