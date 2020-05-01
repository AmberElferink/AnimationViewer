#pragma once

#include "pipeline.h"

namespace AnimationViewer::Graphics {
class PipelineRasterOpenGL : public Pipeline
{
public:
  /// A factory function in the impl class allows for an error to return null
  static std::unique_ptr<Pipeline> create(const CreateInfo& info);
  explicit PipelineRasterOpenGL(uint32_t program);
  ~PipelineRasterOpenGL() override;

  void set_uniform(uint8_t location, UniformType type, uint32_t count, const void* value) override;
  void bind() override;

  uint32_t get_native_handle() const override;

private:
  const uint32_t program_;
};
} // namespace AnimationViewer::Graphics
