#include "resource.h"

#include <ANMFile.h>
#include <L3DFile.h>
#include <ezc3d.h>
#include <glm/matrix.hpp>

#include "renderer.h"

#include "private_impl/graphics/indexed_mesh.h"

using namespace AnimationViewer;

namespace {
enum class FileType
{
  /// Lionhead Studio Mesh
  L3D,
  /// Lionhead Studio Animation
  ANM,
  /// Biomechanics standard file format
  C3D,
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
  // Check for ANM
  if (ext == ".anm" || ext == ".Anm" || ext == ".ANm" || ext == ".aNm" || ext == ".aNM" ||
      ext == ".anM" || ext == ".AnM" || ext == ".ANM") {
    return FileType::ANM;
  }
  // Check for C3D
  if (ext == ".c3d" || ext == ".C3D" || ext == ".c3D" || ext == ".c3d") {
    FILE* file = fopen(path.string().c_str(), "rb");
    char magic_number[2];
    fread(magic_number, sizeof(magic_number), 1, file);
    // Some C3D files have a bunch of 0s at the start of the file, we're not supporting those
    if (magic_number[1] == 0x50) {
      return FileType::C3D;
    }
  }
  return FileType::Unknown;
}
} // namespace

namespace AnimationViewer::Loader {
struct Mesh final : entt::loader<Mesh, Resource::Mesh>
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
    uint32_t vertex_index = 0, vertex_group_index = 0;

    mesh->vertices.reserve(l3d.GetVertices().size());
    for (size_t i = 0; i < l3d.GetVertices().size(); i++) {
      const auto& vertex = l3d.GetVertices()[i];
      if (l3d.GetLookUpTableData().size() > vertex_group_index &&
          vertex_index >= l3d.GetLookUpTableData()[vertex_group_index].vertexCount) {
        vertex_group_index++;
        vertex_index = 0;
      }

      uint32_t bone_index = 0;
      if (l3d.GetLookUpTableData().size() > vertex_group_index) {
        bone_index = l3d.GetLookUpTableData()[vertex_group_index].boneIndex;
      }

      mesh->vertices.push_back({
        { vertex.position.x, vertex.position.y, vertex.position.z },
        { vertex.normal.x, vertex.normal.y, vertex.normal.z },
        static_cast<float>(bone_index),
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

struct Animation final : entt::loader<Animation, Resource::Animation>
{
  std::shared_ptr<Resource::Animation> load(const std::string& name,
                                            const openblack::anm::ANMFile& anm) const
  {
    auto animation = std::make_shared<Resource::Animation>();
    animation->name = anm.GetHeader().name;
    animation->frame_count = anm.GetHeader().frame_count;
    animation->animation_duration = anm.GetHeader().animation_duration * 1000;
    
    const std::vector<openblack::anm::ANMFrame> &frames = anm.GetKeyframes();
    animation->keyframes.reserve(animation->frame_count * sizeof(Resource::AnimationFrame));
    for (uint32_t i = 0; i < animation->frame_count; i++) {
      Resource::AnimationFrame frame;

      for (auto bone : frames[i].bones) {
        glm::mat4x3 bone4x3mat = glm::make_mat4x3(bone.matrix);
        frame.bones.emplace_back(bone4x3mat);
      }
      frame.time = frames[i].time;
      animation->keyframes.push_back(frame);
    }

    return animation;
  }
};

struct MotionCapture final : entt::loader<MotionCapture, Resource::MotionCapture>
{
  std::shared_ptr<Resource::MotionCapture> load(const std::string& name,
                                                const ezc3d::c3d& c3d) const
  {
    auto mocap = std::make_shared<Resource::MotionCapture>();
    mocap->name = name;

    // TODO: copy data over from c3d to mocap resource

    return mocap;
  }
};
} // namespace AnimationViewer::Loader

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

const entt::cache<Resource::MotionCapture>&
ResourceManager::motion_capture_cache() const
{
  return motion_capture_cache_;
}

std::optional<std::pair<entt::hashed_string, ResourceManager::Type>>
ResourceManager::load_file(const std::filesystem::path& path)
{
  switch (detect_file_type(path)) {
    case FileType::L3D: {
      auto mesh = load_l3d_file(path);
      if (mesh.has_value()) {
        return std::make_pair(*mesh, Type::Mesh);
      }
    } break;
    case FileType::ANM: {
      auto animation = load_anm_file(path);
      if (animation.has_value()) {
        return std::make_pair(*animation, Type::Animation);
      }
    } break;
    case FileType::C3D: {
      auto mocap = load_c3d_file(path);
      if (mocap.has_value()) {
        return std::make_pair(*mocap, Type::MotionCapture);
      }
    } break;
    case FileType::Unknown:
      fprintf(stderr, "[scene]: Unsupported filetype: %s\n", path.c_str());
  }
  return std::nullopt;
}

std::optional<entt::hashed_string>
ResourceManager::load_l3d_file(const std::filesystem::path& path)
{
  openblack::l3d::L3DFile l3d;
  l3d.Open(path.string());
  auto id = entt::hashed_string{ path.string().c_str() };
  mesh_cache_.load<Loader::Mesh>(id, path.filename().string(), l3d);
  return std::make_optional(id);
}

std::optional<entt::hashed_string>
ResourceManager::load_anm_file(const std::filesystem::path& path)
{
  openblack::anm::ANMFile anm;
  anm.Open(path.string());
  auto id = entt::hashed_string{ path.string().c_str() };
  animation_cache_.load<Loader::Animation>(id, path.filename().string(), anm);
  return std::make_optional(id);
}

std::optional<entt::hashed_string>
ResourceManager::load_c3d_file(const std::filesystem::path& path)
{
  ezc3d::c3d c3d(path.string());
  auto id = entt::hashed_string{ path.string().c_str() };
  motion_capture_cache_.load<Loader::MotionCapture>(id, path.filename().string(), c3d);
  return std::make_optional(id);
}
