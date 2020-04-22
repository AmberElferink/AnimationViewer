#pragma once

#include <chrono>

#include <glm/mat4x4.hpp>

union SDL_Event;

namespace AnimationViewer {

class Camera
{
public:
  Camera(const glm::mat4& matrix, float y_fov) noexcept;

  /// Update camera state based on SDL events
  void process_event(const SDL_Event& event, std::chrono::microseconds& dt);

private:
  glm::mat4 matrix_;
  float y_fov_;
};
} // namespace AnimationViewer
