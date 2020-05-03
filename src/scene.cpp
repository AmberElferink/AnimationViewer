#include "scene.h"

#include <string>

#include <glm/trigonometric.hpp>

using namespace AnimationViewer;

std::unique_ptr<Scene>
Scene::create()
{
  return std::unique_ptr<Scene>(new Scene());
}

Scene::Scene()
  : default_camera_(glm::vec3{ 0.0f, 1.0f, 3.0f }, 0, 0, glm::radians(80.0f))
{}
Scene::~Scene() = default;

void
Scene::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  if (cameras_.empty() || active_camera_ > cameras_.size()) {
    default_camera_.process_event(event, dt);
  } else {
    cameras_[active_camera_].process_event(event, dt);
  }
}

const Camera&
Scene::active_camera() const
{
  if (cameras_.empty() || active_camera_ > cameras_.size()) {
    return default_camera_;
  }
  return cameras_[active_camera_];
}
