#pragma once

#include <chrono>
#include <memory>

union SDL_Event;

namespace AnimationViewer {
class Scene
{
public:
  static std::unique_ptr<Scene> create();
  virtual ~Scene();

  /// Update scene based on SDL events
  void process_event(const SDL_Event& event, std::chrono::microseconds& dt);

protected:
  Scene();
};
} // namespace AnimationViewer
