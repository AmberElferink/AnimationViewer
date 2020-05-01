#include "pipeline.h"

#include "pipelines/pipeline_raster_opengl.h"

using AnimationViewer::Graphics::Pipeline;

std::unique_ptr<Pipeline>
Pipeline::create(Type type, const Pipeline::CreateInfo& info)
{
  switch (type) {
    case Pipeline::Type::RasterOpenGL:
      return PipelineRasterOpenGL::create(info);
  }
  return nullptr;
}
