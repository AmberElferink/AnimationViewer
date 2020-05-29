#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <entt/entity/registry.hpp>

#include "camera.h"

union SDL_Event;

namespace AnimationViewer {
class ResourceManager;

namespace Components {
struct Mesh
{
  entt::hashed_string id;
};
struct Armature
{
  std::vector<glm::mat4> joints;
};
struct Animation
{
  entt::hashed_string id;
  uint32_t dummy; // TODO: replace me with things an animation needs, such as current animation time
};
} // namespace Components

class Scene
{
public:
  static std::unique_ptr<Scene> create();
  virtual ~Scene();

  /// Update scene based on SDL events
  void process_event(const SDL_Event& event, std::chrono::microseconds& dt);
  void add_mesh(const entt::hashed_string& id,
                const glm::ivec2& screen_space_position,
                AnimationViewer::ResourceManager& resource_manager);

  void run(ResourceManager& resource_manager);

  /// A scene can have any number of cameras including zero
  /// This returns the camera selected for rendering or a default camera
  /// if there are no cameras in the scene.
  const Camera& active_camera() const;
  const entt::registry& registry() const;

protected:
  Scene();

private:
  /// Default read-only camera used only if there is no camera in the scene
  /// selected
  Camera default_camera_;
  uint32_t active_camera_;
  std::vector<Camera> cameras_;

  mutable entt::registry registry_;
};
} // namespace AnimationViewer
