#pragma once

#include "pipeline.h"

namespace AnimationViewer::Graphics {
class PipelineRasterOpenGL : public Pipeline
{
public:
  explicit PipelineRasterOpenGL(uint32_t program);
  ~PipelineRasterOpenGL() override;

  void bind() override;
  uint32_t get_native_handle() const override;

  /// A factory function in the impl class allows for an error to return null
  static std::unique_ptr<Pipeline> create(const CreateInfo& info);

private:
  const uint32_t program;
};
} // namespace AnimationViewer::Graphics
