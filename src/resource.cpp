#include "resource.h"

#include <L3DFile.h>

#include "renderer.h"

#include "private_impl/graphics/indexed_mesh.h"

using namespace AnimationViewer;

namespace {
enum class FileType
{
  // Lionhead Studio Mesh
  L3D,
  Unknown
};

FileType
detect_file_type(const std::filesystem::path& path)
{
  auto ext = path.extension();
  // Check for L3D
  if (ext == ".l3d" || ext == ".L3D" || ext == ".l3D" || ext == ".L3d") {
    FILE* file = fopen(path.c_str(), "rb");
    char magic_number[3];
    fread(magic_number, sizeof(magic_number), sizeof(magic_number), file);
    if (magic_number[0] == 'L' && magic_number[1] == '3' && magic_number[2] == 'D') {
      return FileType::L3D;
    }
  }
  return FileType::Unknown;
}
} // namespace

struct MeshLoader final : entt::loader<MeshLoader, MeshResource>
{
  std::shared_ptr<MeshResource> load(const std::string& name,
                                     const openblack::l3d::L3DFile& l3d) const
  {
    auto mesh = std::make_shared<MeshResource>();
    mesh->name = name;
    mesh->vertices.reserve(l3d.GetVertices().size() * 6);
    for (const auto& vertex : l3d.GetVertices()) {
      mesh->vertices.push_back(vertex.position.x);
      mesh->vertices.push_back(vertex.position.y);
      mesh->vertices.push_back(vertex.position.z);
      mesh->vertices.push_back(vertex.normal.x);
      mesh->vertices.push_back(vertex.normal.y);
      mesh->vertices.push_back(vertex.normal.z);
    }
    mesh->indices = l3d.GetIndices();

    return mesh;
  }
};

std::unique_ptr<ResourceManager>
ResourceManager::create()
{
  entt::cache<MeshResource> mesh_cache{};
  return std::unique_ptr<ResourceManager>(new ResourceManager(std::move(mesh_cache)));
}

ResourceManager::ResourceManager(entt::cache<MeshResource>&& mesh_cache)
  : mesh_cache_(std::move(mesh_cache))
{}

ResourceManager::~ResourceManager() = default;

void
ResourceManager::upload_dirty_buffers(Graphics::Renderer& renderer)
{
  mesh_cache_.each([&renderer](MeshResource& res) {
    if (!res.gpu_resource) {
      res.gpu_resource = renderer.upload_mesh(res.vertices, res.indices);
    }
  });
}

const entt::cache<MeshResource>&
ResourceManager::mesh_cache() const
{
  return mesh_cache_;
}

std::optional<entt::hashed_string>
ResourceManager::load_file(const std::filesystem::path& path)
{
  switch (detect_file_type(path)) {
    case FileType::L3D:
      return load_l3d_file(path);
    case FileType::Unknown:
      fprintf(stderr, "[scene]: Unsupported filetype: %s\n", path.c_str());
      return std::nullopt;
  }
}

std::optional<entt::hashed_string>
ResourceManager::load_l3d_file(const std::filesystem::path& path)
{
  openblack::l3d::L3DFile l3d;
  l3d.Open(path.string());
  auto id = entt::hashed_string{ path.string().c_str() };
  mesh_cache_.load<MeshLoader>(id, path.filename().string(), l3d);
  return std::make_optional(id);
}