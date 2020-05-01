#pragma once

#include <chrono>

#include <glm/mat4x4.hpp>

union SDL_Event;

namespace AnimationViewer {

class Camera
{
public:
  Camera(const glm::vec3& origin, float yaw, float pitch, float fov_y) noexcept;

  /// Update camera state based on SDL events
  void process_event(const SDL_Event& event, std::chrono::microseconds& dt);
  float fov_y() const;
  glm::vec3 origin() const;
  glm::mat4 perspective(float aspect) const;
  glm::mat4 matrix() const;

private:
  glm::vec3 origin_;
  float yaw_;
  float pitch_;
  float fov_y_;
  const float near_;
  const float far_;
};
} // namespace AnimationViewer
