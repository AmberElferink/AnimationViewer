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
    FILE* file = fopen(path.string().c_str(), "rb");
    char magic_number[3];
    fread(magic_number, sizeof(magic_number), 1, file);
    if (magic_number[0] == 'L' && magic_number[1] == '3' && magic_number[2] == 'D') {
      return FileType::L3D;
    }
  }
  return FileType::Unknown;
}
} // namespace

struct MeshLoader final : entt::loader<MeshLoader, Resource::Mesh>
{
  std::shared_ptr<Resource::Mesh> load(const std::string& name,
                                       const openblack::l3d::L3DFile& l3d) const
  {
    auto mesh = std::make_shared<Resource::Mesh>();
    mesh->name = name;

    mesh->bones.reserve(l3d.GetBones().size());
    for (const auto& bone : l3d.GetBones()) {
      glm::mat3 orient = glm::make_mat3(bone.orientation);

      mesh->bones.push_back({
        bone.parent,
        bone.firstChild,
        bone.rightSibling,
        { bone.position.x, bone.position.y, bone.position.z },
        orient,
      });
    }

    // Add all vertices
    int vertex_index = 0, vertex_group_index = 0;

    mesh->vertices.reserve(l3d.GetVertices().size());
    for (size_t i = 0; i < l3d.GetVertices().size(); i++) {
      const auto& vertex = l3d.GetVertices()[i];
      if (vertex_index >= l3d.GetLookUpTableData()[vertex_group_index].vertexCount) {
        vertex_group_index++;
        vertex_index = 0;
      }

      uint32_t bone_index = l3d.GetLookUpTableData()[vertex_group_index].boneIndex;

      mesh->vertices.push_back({
        { vertex.position.x, vertex.position.y, vertex.position.z },
        { vertex.normal.x, vertex.normal.y, vertex.normal.z },
        (float)bone_index,
      });

      vertex_index++;
    }

    // Correctly input Indices
    uint32_t indices_index = 0, vertices_offset = 0, indices_group = 0;

    mesh->indices.reserve(l3d.GetIndices().size());
    for (size_t i = 0; i < l3d.GetIndices().size(); i++) {
      if (indices_index >= l3d.GetPrimitiveHeaders()[indices_group].numTriangles * 3) {
        vertices_offset += l3d.GetPrimitiveHeaders()[indices_group].numVertices;
        indices_index = 0;
        indices_group++;
      }

      mesh->indices.push_back(l3d.GetIndices()[i] + vertices_offset);

      indices_index++;
    }

    return mesh;
  }
};

std::unique_ptr<ResourceManager>
ResourceManager::create()
{
  entt::cache<Resource::Mesh> mesh_cache{};
  entt::cache<Resource::Animation> animation_cache{};
  return std::unique_ptr<ResourceManager>(
    new ResourceManager(std::move(mesh_cache), std::move(animation_cache)));
}

ResourceManager::ResourceManager(entt::cache<Resource::Mesh>&& mesh_cache,
                                 entt::cache<Resource::Animation>&& animation_cache)
  : mesh_cache_(std::move(mesh_cache))
  , animation_cache_(std::move(animation_cache))
{}

ResourceManager::~ResourceManager() = default;

void
ResourceManager::upload_dirty_buffers(Graphics::Renderer& renderer)
{
  mesh_cache_.each([&renderer](Resource::Mesh& res) {
    if (!res.gpu_resource) {
      res.gpu_resource = renderer.upload_mesh(res.vertices, res.indices);
    }
  });
}

const entt::cache<Resource::Mesh>&
ResourceManager::mesh_cache() const
{
  return mesh_cache_;
}

const entt::cache<Resource::Animation>&
ResourceManager::animation_cache() const
{
  return animation_cache_;
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
