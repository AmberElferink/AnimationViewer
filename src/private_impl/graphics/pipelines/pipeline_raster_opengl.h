#pragma once

#include "pipeline.h"

namespace AnimationViewer::Graphics {
class PipelineRasterOpenGL : public Pipeline
{
public:
  /// A factory function in the impl class allows for an error to return null
  static std::unique_ptr<Pipeline> create(const CreateInfo& info);
  PipelineRasterOpenGL(uint32_t program,
                       uint32_t winding_order,
                       std::optional<uint32_t> cull_mode,
                       bool depth_write,
                       std::optional<uint32_t> depth_test,
                       bool blend);
  ~PipelineRasterOpenGL() override;

  void set_uniform(uint8_t location, UniformType type, uint32_t count, const void* value) override;
  void bind() override;

  uint32_t get_native_handle() const override;

private:
  const uint32_t program_;
  const uint32_t winding_order_;
  const std::optional<uint32_t> cull_mode_;
  const bool depth_write_;
  const std::optional<uint32_t> depth_test_;
  const bool blend_;
};
} // namespace AnimationViewer::Graphics
