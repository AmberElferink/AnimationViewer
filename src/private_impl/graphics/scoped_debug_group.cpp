#include "scoped_debug_group.h"

#include <glad/glad.h>

using namespace AnimationViewer::Graphics;

ScopedDebugGroup::ScopedDebugGroup(const std::string_view& label) noexcept
{
  if (glPushDebugGroup && glPopDebugGroup) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label.data());
  }
}
ScopedDebugGroup::~ScopedDebugGroup()
{
  if (glPushDebugGroup && glPopDebugGroup) {
    glPopDebugGroup();
  }
}
