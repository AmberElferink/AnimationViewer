#pragma once

#include <string_view>

namespace AnimationViewer::Graphics {
class ScopedDebugGroup
{
public:
  explicit ScopedDebugGroup(const std::string_view& label) noexcept;
  ~ScopedDebugGroup();
};
} // namespace AnimationViewer::Graphics
