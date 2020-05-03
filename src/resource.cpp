#include "resource.h"

#include "private_impl/graphics/indexed_mesh.h"

using namespace AnimationViewer;

struct MeshLoader final : entt::loader<MeshLoader, MeshResource>
{
  std::shared_ptr<MeshResource> load() const { return std::make_shared<MeshResource>(); }
};

std::unique_ptr<ResourceManager>
ResourceManager::create()
{
  entt::cache<MeshResource> mesh_cache{};
  mesh_cache.load<MeshLoader>(entt::hashed_string{ "my/resource/identifier" });
  return std::unique_ptr<ResourceManager>(new ResourceManager(std::move(mesh_cache)));
}

ResourceManager::ResourceManager(entt::cache<MeshResource>&& mesh_cache)
  : mesh_cache_(std::move(mesh_cache))
{}

ResourceManager::~ResourceManager() = default;

void
ResourceManager::upload_dirty_buffers(Graphics::Renderer& renderer)
{
  mesh_cache_.each([](MeshResource& res) {
    if (!res.gpu_resource) {
      res.gpu_resource = Graphics::IndexedMesh::create_box();
    }
  });
}

const entt::cache<MeshResource>&
ResourceManager::mesh_cache() const
{
  return mesh_cache_;
}
