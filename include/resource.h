#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace AnimationViewer {
namespace Graphics {
struct IndexedMesh;
class Renderer;
} // namespace Graphics

struct vertex_t
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 bone_id; // Blend x and y by z
};

struct bone_t
{
  std::string name;
  uint32_t parent;
  uint32_t firstChild;
  uint32_t rightSibling;
  glm::vec3 position;
  glm::mat3 orientation;
};

namespace Resource {
struct Mesh
{
  Mesh() = default;
  std::string name;
  std::optional<glm::mat4> default_matrix;
  std::vector<vertex_t> vertices;
  std::vector<bone_t> bones;
  std::vector<uint16_t> indices;
  std::unique_ptr<Graphics::IndexedMesh> gpu_resource;
};

struct AnimationFrame
{
  uint32_t time;
  std::vector<glm::mat4> bones; // bones trans_rot (no bottom row)
};

struct Animation
{
  Animation() = default;
  std::string name;
  float frame_rate;
  uint32_t frame_count;
  uint32_t animation_duration;
  std::vector<std::string> joint_names;
  std::vector<AnimationFrame> keyframes;
};

struct MotionCapture
{
  MotionCapture() = default;
  std::string name;
  float frame_rate;
  uint32_t point_count;
  /// Flat array of frame count * point count, with all points in one frame sequential
  std::vector<glm::vec3> frame_points;
};
} // namespace Resource

class ResourceManager
{
public:
  enum Type
  {
    Mesh = 1u << 0u,
    Animation = 1u << 1u,
    MotionCapture = 1u << 2u,
  };

  static std::unique_ptr<ResourceManager> create();
  virtual ~ResourceManager();

  /// Use the rendering device/context to upload cpu_resources into gpu_resources
  void upload_dirty_buffers(Graphics::Renderer& renderer);

  /// Load file from path and detect type before loading as mesh or animation
  ///
  /// @path is the path of the file to load
  std::vector<std::pair<entt::hashed_string, Type>> load_file(const std::filesystem::path& path);

  const entt::cache<Resource::Mesh>& mesh_cache() const;
  const entt::cache<Resource::Animation>& animation_cache() const;
  const entt::cache<Resource::MotionCapture>& motion_capture_cache() const;

protected:
  ResourceManager(entt::cache<Resource::Mesh>&& mesh_cache,
                  entt::cache<Resource::Animation>&& animation_cache);

  std::optional<entt::hashed_string> load_l3d_file(const std::filesystem::path& path);
  std::vector<std::pair<entt::hashed_string, Type>> load_fbx_file(const std::filesystem::path& path);
  std::optional<entt::hashed_string> load_anm_file(const std::filesystem::path& path);
  std::optional<entt::hashed_string> load_bvh_file(const std::filesystem::path& path);
  std::optional<entt::hashed_string> load_c3d_file(const std::filesystem::path& path);
  std::vector<std::pair<entt::hashed_string, Type>> load_assimp_file(const std::filesystem::path& path, bool skip_meshes);

private:
  entt::cache<Resource::Mesh> mesh_cache_;
  entt::cache<Resource::Animation> animation_cache_;
  entt::cache<Resource::MotionCapture> motion_capture_cache_;
};
} // namespace AnimationViewer
