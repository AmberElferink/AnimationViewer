#pragma once

#include <memory>
#include <vector>

#include <entt/entt.hpp>

namespace AnimationViewer {
namespace Graphics {
struct IndexedMesh;
class Renderer;
} // namespace Graphics

struct MeshResource {
  MeshResource() = default;
  std::vector<uint8_t> cpu_resource;
  std::unique_ptr<Graphics::IndexedMesh> gpu_resource;
};

class ResourceManager
{
public:
  static std::unique_ptr<ResourceManager> create();
  virtual ~ResourceManager();

  /// Use the rendering device/context to upload cpu_resources into gpu_resources
  void upload_dirty_buffers(Graphics::Renderer& renderer);

  const entt::cache<MeshResource>& mesh_cache() const;

protected:
  explicit ResourceManager(entt::cache<MeshResource>&& mesh_cache);

private:
  entt::cache<MeshResource> mesh_cache_;
};
} // namespace AnimationViewer
