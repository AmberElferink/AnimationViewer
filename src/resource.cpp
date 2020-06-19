#include "resource.h"

#include <stack>
#include <unordered_map>

#include <ANMFile.h>
#include <Data.h>
#include <Frame.h>
#include <Header.h>
#include <L3DFile.h>
#include <bvh.h>
#include <bvhloader.h>
#include <ezc3d.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>
#include <ofbx.h>

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
  /// Autodesk FBX
  FBX,
  /// Biovision Hierarchy animation file
  BVH,
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
  // Check for FBX
  if (ext == ".fbx" || ext == ".Fbx" || ext == ".FBx" || ext == ".fBx" || ext == ".fBX" ||
      ext == ".fbX" || ext == ".FbX" || ext == ".FBX") {
    FILE* file = fopen(path.string().c_str(), "rb");
    char magic_number[21];
    fread(magic_number, 1, sizeof(magic_number), file);
    magic_number[20] = '\0';
    if (strcmp(magic_number, "Kaydara FBX Binary  ") == 0) {
      return FileType::FBX;
    }
  }
  // Check for BVH
  if (ext == ".bvh" || ext == ".Bvh" || ext == ".BVh" || ext == ".bVh" || ext == ".bVH" ||
      ext == ".bvH" || ext == ".BvH" || ext == ".BVH") {
    FILE* file = fopen(path.string().c_str(), "rb");
    char magic_number[10];
    fread(magic_number, 1, sizeof(magic_number), file);
    magic_number[9] = '\0';
    if (strcmp(magic_number, "HIERARCHY") == 0) {
      return FileType::BVH;
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
        "",
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

  std::shared_ptr<Resource::Mesh> load(const ofbx::Mesh* mesh) const
  {
    auto mesh_resource = std::make_shared<Resource::Mesh>();
    mesh_resource->name = mesh->name;

    if (mesh->getPose()) {
      mesh_resource->default_matrix =
        std::make_optional<glm::mat4>(glm::make_mat4(mesh->getPose()->getMatrix().m));
    }

    auto geometry = mesh->getGeometry();
    auto geom_scale = glm::vec3(1.0f);
    if (mesh_resource->default_matrix) {
      glm::vec3 position;
      glm::vec3 skew;
      glm::vec4 perspective;
      glm::quat orientation;
      glm::decompose(
        *mesh_resource->default_matrix, geom_scale, orientation, position, skew, perspective);
    }

    mesh_resource->vertices.resize(geometry->getVertexCount());
    for (int i = 0; i < geometry->getVertexCount(); ++i) {
      mesh_resource->vertices[i].position.x = geometry->getVertices()[i].x;
      mesh_resource->vertices[i].position.y = geometry->getVertices()[i].y;
      mesh_resource->vertices[i].position.z = geometry->getVertices()[i].z;
      mesh_resource->vertices[i].normal.x = geometry->getNormals()[i].x;
      mesh_resource->vertices[i].normal.y = geometry->getNormals()[i].y;
      mesh_resource->vertices[i].normal.z = geometry->getNormals()[i].z;
    }

    mesh_resource->indices.resize(geometry->getIndexCount());
    for (int i = 0; i < geometry->getIndexCount(); ++i) {
      if (geometry->getFaceIndices()[i] >= 0) {
        mesh_resource->indices[i] = geometry->getFaceIndices()[i];
      } else {
        mesh_resource->indices[i] = -(geometry->getFaceIndices()[i] + 1);
      }
    }

    // Object->id, mesh_resource->bones index
    std::unordered_map<uint64_t, uint32_t> seen_links;
    std::stack<const ofbx::Object*> branch;

    auto skin = geometry->getSkin();
    if (skin && skin->getClusterCount() > 0) {
      for (int i = 0; i < skin->getClusterCount(); ++i) {
        auto cluster = skin->getCluster(i);
        assert(cluster->getIndicesCount() == cluster->getWeightsCount());

        // Move up the tree until we encounter a node that we've seen already
        const ofbx::Object* parent_node = nullptr;
        for (auto walker = cluster->getLink();
             walker != nullptr && walker->getType() == ofbx::Object::Type::LIMB_NODE;
             walker = walker->getParent()) {
          if (seen_links.count(walker->id)) {
            // Unexplored subtree finished
            parent_node = walker;
            break;
          }
          branch.push(walker);
        }

        // Now we have the lowest parent to this branch in seen_node and the branch
        // we can start attaching the branch to seen_node (or creating a tree if
        // seen_node is null.

        bool is_root = parent_node == nullptr;
        uint32_t parent_id = 0;
        if (!is_root) {
          parent_id = parent_node->id;
        }
        // Insert node in tree list
        auto orientation = glm::mat3(1.0);
        if (!is_root) {
          orientation = mesh_resource->bones[seen_links[parent_node->id]].orientation;
        }
        while (!branch.empty()) {
          uint32_t current_index = mesh_resource->bones.size();
          auto& bone = mesh_resource->bones.emplace_back();
          bone.firstChild = std::numeric_limits<uint32_t>::max();
          bone.rightSibling = std::numeric_limits<uint32_t>::max();
          bone.name = branch.top()->name;
          auto rotation = branch.top()->getLocalRotation();
          switch (branch.top()->getRotationOrder()) {
            case ofbx::RotationOrder::EULER_XYZ:
              bone.orientation = glm::mat3(glm::eulerAngleZYX(
                glm::radians(rotation.z), glm::radians(rotation.y), glm::radians(rotation.x)));
              break;
            case ofbx::RotationOrder::EULER_XZY:
            case ofbx::RotationOrder::EULER_YZX:
            case ofbx::RotationOrder::EULER_YXZ:
            case ofbx::RotationOrder::EULER_ZXY:
            case ofbx::RotationOrder::EULER_ZYX:
            case ofbx::RotationOrder::SPHERIC_XYZ:
            default:
              assert(false);
          }
          glm::vec3 scale(1.0f);
          {
            auto transform = cluster->getTransformLinkMatrix();
            glm::mat4 matrix = glm::make_mat4(&transform.m[0]);
            glm::vec3 position;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::quat orientation_tmp;
            glm::decompose(matrix, scale, orientation_tmp, position, skew, perspective);
          }
          auto translation = branch.top()->getLocalTranslation();
          //          bone.position += glm::make_vec3(&translation.x);
          bone.position = glm::make_vec3(&translation.x);
          bone.position *= scale / geom_scale;
          orientation = bone.orientation * orientation;
          //          bone.orientation = orientation;
          // Set seen node index as parent
          if (is_root) {
            bone.parent = std::numeric_limits<uint32_t>::max();
            is_root = false;
          } else {
            bone.parent = seen_links[parent_id];
            // Insert self into parent's child linked list
            if (mesh_resource->bones[bone.parent].firstChild ==
                std::numeric_limits<uint32_t>::max()) {
              mesh_resource->bones[bone.parent].firstChild = current_index;
            } else {
              uint32_t walker;
              for (walker = bone.parent;
                   mesh_resource->bones[walker].rightSibling < std::numeric_limits<uint32_t>::max();
                   walker = mesh_resource->bones[walker].rightSibling) {
              }
              mesh_resource->bones[bone.parent].rightSibling = current_index;
            }
          }
          // Add to seen_links, pop from stack
          seen_links.emplace(branch.top()->id, current_index);
          parent_id = branch.top()->id;
          branch.pop();
        }

for (int j = 0; j < cluster->getIndicesCount(); ++j) {
  assert(cluster->getIndices()[j] < geometry->getVertexCount());
  // TODO: We don't support blending yet
  assert(cluster->getWeights()[i] <= 1);

  auto& vertex = mesh_resource->vertices[cluster->getIndices()[j]];
  vertex.bone_id = seen_links[cluster->getLink()->id];
}
      }
      // Convert from absolute to relative to joint
      for (auto& vertex : mesh_resource->vertices) {
        glm::mat4 matrix(1.0f);
        for (uint32_t bone_id = vertex.bone_id; bone_id < std::numeric_limits<uint32_t>::max();
          bone_id = mesh_resource->bones[bone_id].parent) {
          auto& bone = mesh_resource->bones[bone_id];
          glm::mat4 rot = glm::mat4(bone.orientation);
          glm::mat4 trans = glm::translate(bone.position);
          matrix = trans * rot * matrix;
        }
        glm::vec3 translation = -matrix[3];
        glm::mat3 rotation = glm::transpose(matrix);
        vertex.position = rotation * (vertex.position + translation);
      }
    }
    return mesh_resource;
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
    animation->frame_time =
      animation->frame_count / static_cast<float>(animation->animation_duration);
    animation->is_relative = false;

    const std::vector<openblack::anm::ANMFrame>& frames = anm.GetKeyframes();
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

  std::shared_ptr<Resource::Animation> load(const std::string& name, const k::Bvh& bvh) const
  {
    auto animation = std::make_shared<Resource::Animation>();

    auto root = bvh.getRootJoint();
    auto& motion_data = bvh.getMotionData();

    animation->name = name;
    animation->frame_count = bvh.getNumFrames();
    animation->frame_time = motion_data.frame_time / 1000;
    animation->animation_duration = animation->frame_time * motion_data.num_frames;
    animation->is_relative = true;

    animation->keyframes.reserve(animation->frame_count);
    for (uint32_t i = 0; i < animation->frame_count; i++) {
      Resource::AnimationFrame frame;

      uint32_t index = 0;
      ParseBVH(frame, animation->joint_names, motion_data, root, i, index);
      frame.time = motion_data.frame_time * i;

      animation->keyframes.push_back(frame);
    }

    return animation;
  }

  void ParseBVH(Resource::AnimationFrame& frame, std::map<std::string, uint32_t>& joint_names, const k::MOTION& motion_data, const k::JOINT* joint, uint32_t frame_nr, uint32_t& index) const {
    // BVH-Loader interprets endsites as joints, we don't want this so ignore these joints.
    if (joint->name == "EndSite") {
      index--;
      return;
    }

    auto channel_start_index = (frame_nr * motion_data.num_motion_channels) + joint->channel_start;
    glm::mat4 transformedMatrix = joint->matrix;

    //transformedMatrix = glm::translate(glm::mat4(1.0), glm::vec3(
    //  joint->offset.x, 
    //  joint->offset.y, 
    //  joint->offset.z));

    /*if (strcmp(joint->name.c_str(), "LeftLeg") == 0 && frame_nr == 1){
      __debugbreak();
    }*/

    for (uint32_t j = 0; j < joint->num_channels; j++) {
      const short& channel = joint->channels_order[j];

      float value = motion_data.data[channel_start_index + j];

      // X position
//      if (channel & 0x01) {
//        transformedMatrix = glm::translate(transformedMatrix, glm::vec3(value, 0, 0));
//      }
//      // Y position
//      if (channel & 0x02) {
//        transformedMatrix = glm::translate(transformedMatrix, glm::vec3(0, value, 0));
//      }
//      // Z position
//      if (channel & 0x04) {
//        transformedMatrix = glm::translate(transformedMatrix, glm::vec3(0, 0, value));
//      }
      // X rotation
      if (channel & 0x20) {
        transformedMatrix = glm::rotate(transformedMatrix, glm::radians(value), glm::vec3(1, 0, 0));
      }
      // Y rotation
      if (channel & 0x40) {
        transformedMatrix = glm::rotate(transformedMatrix, glm::radians(value), glm::vec3(0, 1, 0));
      }
      // Z rotation
      if (channel & 0x10) {
        transformedMatrix = glm::rotate(transformedMatrix, glm::radians(value), glm::vec3(0, 0, 1));
      }
    }

    frame.bones.emplace_back(transformedMatrix);
    joint_names[joint->name] = index;
    for (auto childjoint : joint->children) {
      index++;
      ParseBVH(frame, joint_names, motion_data, childjoint, frame_nr, index);
    }
  }
};

struct MotionCapture final : entt::loader<MotionCapture, Resource::MotionCapture>
{
  std::shared_ptr<Resource::MotionCapture> load(const std::string& name,
                                                const ezc3d::c3d& c3d) const
  {
    auto mocap = std::make_shared<Resource::MotionCapture>();
    mocap->name = name;
    mocap->frame_rate = c3d.header().frameRate();
    mocap->point_count = c3d.header().nb3dPoints();
    const auto& data = c3d.data();
    mocap->frame_points.resize(mocap->point_count * data.nbFrames());

    for (uint32_t i = 0; i < data.nbFrames(); ++i) {
      const auto& frame = data.frame(i);
      const auto& points = frame.points();
      assert(points.nbPoints() == mocap->point_count);
      for (uint32_t j = 0; j < mocap->point_count; ++j) {
        mocap->frame_points[i * mocap->point_count + j].x = points.point(j).x();
        mocap->frame_points[i * mocap->point_count + j].y = points.point(j).z();
        mocap->frame_points[i * mocap->point_count + j].z = points.point(j).y();
      }
    }

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

std::vector<std::pair<entt::hashed_string, ResourceManager::Type>>
ResourceManager::load_file(const std::filesystem::path& path)
{
  switch (detect_file_type(path)) {
    case FileType::L3D: {
      auto mesh = load_l3d_file(path);
      if (mesh.has_value()) {
        return { std::make_pair(*mesh, Type::Mesh) };
      }
    } break;
    case FileType::FBX: {
      return load_fbx_file(path);
    } break;
    case FileType::ANM: {
      auto animation = load_anm_file(path);
      if (animation.has_value()) {
        return { std::make_pair(*animation, Type::Animation) };
      }
    } break;
    case FileType::BVH: {
      auto animation = load_bvh_file(path);
      if (animation.has_value()) {
        return { std::make_pair(*animation, Type::Animation) };
      }
    } break;
    case FileType::C3D: {
      auto mocap = load_c3d_file(path);
      if (mocap.has_value()) {
        return { std::make_pair(*mocap, Type::MotionCapture) };
      }
    } break;
    case FileType::Unknown:
      fprintf(stderr, "[scene]: Unsupported filetype: %s\n", path.c_str());
  }
  return {};
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

std::vector<std::pair<entt::hashed_string, ResourceManager::Type>>
ResourceManager::load_fbx_file(const std::filesystem::path& path)
{
  std::vector<uint8_t> contents;
  FILE* fp = fopen(path.string().c_str(), "rb");
  fseek(fp, 0, SEEK_END);
  contents.resize(ftell(fp));
  fseek(fp, 0, SEEK_SET);
  fread(contents.data(), 1, contents.size(), fp);

  auto fbx = ofbx::load(contents.data(),
                        contents.size(),
                        ofbx::LoadFlags::TRIANGULATE | ofbx::LoadFlags::IGNORE_BLEND_SHAPES);

  std::vector<std::pair<entt::hashed_string, ResourceManager::Type>> result;

  uint32_t unnamed_count = 0;
  for (int i = 0; i < fbx->getMeshCount(); ++i) {
    const ofbx::Mesh* mesh = fbx->getMesh(i);
    std::string name = mesh->name;
    if (name.empty()) {
      unnamed_count++;
      name = path.string() + " unnamed " + std::to_string(unnamed_count);
    }
    auto id = entt::hashed_string{ name.c_str() };
    mesh_cache_.load<Loader::Mesh>(id, mesh);
    result.emplace_back(id, Type::Mesh);
  }

  return result;
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
ResourceManager::load_bvh_file(const std::filesystem::path& path)
{
  k::Bvh bvh;
  {
    k::BvhLoader bvhLoader;
    bvhLoader.load(&bvh, path.string());
  }
  auto id = entt::hashed_string{ path.string().c_str() };
  animation_cache_.load<Loader::Animation>(id, path.filename().string(), bvh);
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
