#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>
#include <array>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace AnimationViewer {
namespace Graphics {
struct IndexedMesh;
class Renderer;
} // namespace Graphics


struct vertex_t {
  glm::vec3 position;
  glm::vec3 normal;
  float bone_id;
};

struct bone_t {
    uint32_t parent;
    uint32_t firstChild;
    uint32_t rightSibling;
    glm::vec3 position;
    glm::mat3 orientation;
};

struct MeshResource {
  MeshResource() = default;
  std::string name;
  std::vector<vertex_t> vertices;
  std::vector<bone_t> bones;
  std::vector<uint16_t> indices;
  std::unique_ptr<Graphics::IndexedMesh> gpu_resource;
};

class ResourceManager
{
public:
  static std::unique_ptr<ResourceManager> create();
  virtual ~ResourceManager();

  /// Use the rendering device/context to upload cpu_resources into gpu_resources
  void upload_dirty_buffers(Graphics::Renderer& renderer);

  /// Load file from path and detect type before loading as mesh or animation
  ///
  /// @path is the path of the file to load
  std::optional<entt::hashed_string> load_file(const std::filesystem::path& path);

  const entt::cache<MeshResource>& mesh_cache() const;

protected:
  explicit ResourceManager(entt::cache<MeshResource>&& mesh_cache);

  std::optional<entt::hashed_string> load_l3d_file(const std::filesystem::path& path);

private:
  entt::cache<MeshResource> mesh_cache_;
 
};
} // namespace AnimationViewer
