#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include <entt/entity/registry.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

union SDL_Event;

namespace AnimationViewer {
class ResourceManager;

namespace Components {
struct Transform
{
  glm::vec3 position;
  glm::quat orientation;
  glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
};
struct Camera
{
  float fov_y;
  float aspect;
  float near;
  float far;
};
struct Sky
{
  glm::vec3 direction_to_sun;
};
struct Mesh
{
  ENTT_ID_TYPE id;
};
struct Armature
{
  std::vector<glm::mat4> joints;
};
struct Animation
{
  ENTT_ID_TYPE id;
  uint32_t current_frame;
  uint32_t current_time = 0;
  bool animating = false;
  bool loop = false;
  std::vector<std::vector<glm::mat4>> transformed_matrices;
};
struct MotionCaptureAnimation
{
  ENTT_ID_TYPE id;
  uint32_t current_frame;
  uint32_t current_time;
  bool animating;
  bool loop;
  float scale;
  float node_size;
  std::vector<std::vector<glm::mat4>> transformed_matrices;
};
} // namespace Components

class Scene
{
public:
  static std::unique_ptr<Scene> create();
  virtual ~Scene();

  void set_default_camera_aspect(float aspect);
  /// Update scene based on SDL events
  void update(ResourceManager& resource_manager, std::chrono::microseconds& dt);
  void process_event(const SDL_Event& event, std::chrono::microseconds& dt);
  void add_mesh(ENTT_ID_TYPE id,
                const std::optional<glm::vec2>& screen_space_position,
                AnimationViewer::ResourceManager& resource_manager);

  /// A scene can have any number of cameras including zero
  /// This returns the camera selected for rendering or a default camera
  /// if there are no cameras in the scene.
  entt::registry& registry();
  [[nodiscard]] const entt::registry& registry() const;
  static Components::Camera default_camera();

protected:
  Scene();

private:
  entt::registry registry_;
};
} // namespace AnimationViewer
