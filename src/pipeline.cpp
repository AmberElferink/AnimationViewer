#include "pipeline.h"

#include "private_impl/pipelines/pipeline_raster_opengl.h"

using AnimationViewer::Graphics::Pipeline;
using AnimationViewer::Graphics::PipelineCreateInfo;

std::unique_ptr<Pipeline>
Pipeline::create(Type type, const PipelineCreateInfo& info)
{
  switch (type) {
    case Pipeline::Type::RasterOpenGL:
      return PipelineRasterOpenGL::create(info);
  }
  return nullptr;
}
