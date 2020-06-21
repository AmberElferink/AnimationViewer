#include "resource.h"

#include <queue>
#include <stack>
#include <unordered_map>

#include <ANMFile.h>
#include <Data.h>
#include <Frame.h>
#include <Header.h>
#include <L3DFile.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <bvh.h>
#include <bvhloader.h>
#include <ezc3d.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
        "",
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

  std::shared_ptr<Resource::Mesh> load(const std::string& name,
                                       const aiMesh* mesh,
                                       const aiNode* root) const
  {
    auto mesh_resource = std::make_shared<Resource::Mesh>();
    mesh_resource->name = name;

    std::unordered_map<const aiNode*, uint32_t> node_joint_map;
    node_joint_map[nullptr] = std::numeric_limits<uint32_t>::max();
    std::queue<const aiNode*> armature;
    armature.push(root);
    while (!armature.empty()) {
      auto node = armature.front();
      armature.pop();
      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        armature.push(node->mChildren[i]);
      }

      node_joint_map.emplace(node, mesh_resource->bones.size());
      auto& res = mesh_resource->bones.emplace_back();
      res.name = node->mName.C_Str();
      res.parent = node_joint_map[node->mParent];
      res.firstChild = std::numeric_limits<uint32_t>::max();
      res.rightSibling = std::numeric_limits<uint32_t>::max();
      aiQuaterniont<ai_real> rotation;
      aiVector3t<ai_real> position;
      node->mTransformation.DecomposeNoScaling(rotation, position);
      res.position = glm::make_vec3(&position.x);
      res.orientation =
        glm::mat4(glm::normalize(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z)));
    }

    armature.push(root);
    while (!armature.empty()) {
      auto node = armature.front();
      auto joint_id = node_joint_map[node];
      armature.pop();
      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        armature.push(node->mChildren[i]);
      }
      if (node->mNumChildren) {
        mesh_resource->bones[joint_id].firstChild = node_joint_map[node->mChildren[0]];
      }
      joint_id = mesh_resource->bones[joint_id].firstChild;
      for (uint32_t i = 1; i < node->mNumChildren; ++i) {
        mesh_resource->bones[joint_id].rightSibling = node_joint_map[node->mChildren[i]];
      }
    }

    std::unordered_map<std::string, uint32_t> name_joint_map;
    for (auto& [key, value] : node_joint_map) {
      if (key != NULL) {
        name_joint_map.emplace(key->mName.C_Str(), value);
      }
    }

    std::vector<std::pair<const aiBone*, float>> vertex_bone_map(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumBones; ++i) {
      for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
        if (vertex_bone_map[mesh->mBones[i]->mWeights[j].mVertexId].second <
            mesh->mBones[i]->mWeights[j].mWeight) {
          vertex_bone_map[mesh->mBones[i]->mWeights[j].mVertexId].first = mesh->mBones[i];
          vertex_bone_map[mesh->mBones[i]->mWeights[j].mVertexId].second =
            mesh->mBones[i]->mWeights[j].mWeight;
        }
      }
    }

    mesh_resource->vertices.resize(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
      vertex_bone_map[i].first;
      mesh_resource->vertices[i].position = glm::make_vec3(&mesh->mVertices[i].x);
      mesh_resource->vertices[i].normal = glm::make_vec3(&mesh->mNormals[i].x);
      if (vertex_bone_map[i].first) {
        auto v = vertex_bone_map[i].first->mOffsetMatrix * mesh->mVertices[i];
        mesh_resource->vertices[i].position = glm::make_vec3(&v.x);
        mesh_resource->vertices[i].bone_id =
          name_joint_map[vertex_bone_map[i].first->mName.C_Str()];
      } else {
        mesh_resource->vertices[i].position = glm::make_vec3(&mesh->mVertices[i].x);
        mesh_resource->vertices[i].bone_id = 0;
      }
    }

    mesh_resource->indices.resize(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
      assert(mesh->mFaces[i].mNumIndices == 3);
      mesh_resource->indices[3 * i] = mesh->mFaces[i].mIndices[0];
      mesh_resource->indices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
      mesh_resource->indices[3 * i + 2] = mesh->mFaces[i].mIndices[2];
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
    animation->frame_rate =
      animation->frame_count / static_cast<float>(animation->animation_duration);

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
    // TODO fill in these values and remove assert
    assert(false);
    // animation->name =
    animation->frame_count = bvh.getNumFrames();
    // animation->animation_duration =
    // animation->frame_rate =

    animation->keyframes.reserve(animation->frame_count);
    for (uint32_t i = 0; i < animation->frame_count; i++) {
      //  Resource::AnimationFrame frame;
      //
      //  for (auto bone : frames[i].bones) {
      //    glm::mat4x3 bone4x3mat = glm::make_mat4x3(bone.matrix);
      //    frame.bones.emplace_back(bone4x3mat);
      //  }
      //  frame.time = frames[i].time;
      //  animation->keyframes.push_back(frame);
    }

    return animation;
  }

  std::shared_ptr<Resource::Animation> load(const std::string& name,
                                            const aiAnimation* anim,
                                            const aiNode* root) const
  {
    auto animation = std::make_shared<Resource::Animation>();
    animation->name = name;
    animation->frame_count = 0;

    animation->joint_names.resize(anim->mNumChannels);
    for (uint32_t j = 0; j < anim->mNumChannels; ++j) {
      animation->joint_names[j] = anim->mChannels[j]->mNodeName.C_Str();
      auto frame_count = std::max(
        anim->mChannels[j]->mNumPositionKeys,
        std::max(anim->mChannels[j]->mNumRotationKeys, anim->mChannels[j]->mNumScalingKeys));
      if (frame_count > 1) {
        if (animation->frame_count > 1) {
          assert(frame_count == animation->frame_count);
        } else {
          animation->frame_count = anim->mChannels[j]->mNumPositionKeys;
        }
      }
    }
    animation->animation_duration = anim->mDuration * 1e-6f;
    animation->frame_rate = anim->mTicksPerSecond * 1e-6f;

    animation->keyframes.resize(animation->frame_count);
    for (uint32_t i = 0; i < animation->frame_count; ++i) {
      animation->keyframes[i].time = i * anim->mTicksPerSecond;
      animation->keyframes[i].bones.resize(anim->mNumChannels);
    }

    std::unordered_map<std::string, const aiNode*> node_map;
    std::stack<const aiNode*> nodes;
    nodes.push(root);
    while (!nodes.empty()) {
      auto node = nodes.top();
      nodes.pop();
      for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        nodes.push(node->mChildren[i]);
      }
      node_map[node->mName.C_Str()] = node;
    }

    std::vector<std::string> animation_node_names(anim->mNumChannels);
    std::unordered_map<std::string, uint32_t> animation_node_map;
    for (uint32_t j = 0; j < anim->mNumChannels; ++j) {
      auto channel = anim->mChannels[j];
      animation_node_names[j] = channel->mNodeName.C_Str();
      animation_node_map[channel->mNodeName.C_Str()] = j;
    }
    for (uint32_t j = 0; j < anim->mNumChannels; ++j) {
      auto channel = anim->mChannels[j];
      for (uint32_t i = 0; i < animation->frame_count; i++) {
        glm::vec3 translation =
          glm::make_vec3(&channel->mPositionKeys[channel->mNumPositionKeys > 1 ? i : 0].mValue.x);
        glm::quat rotation =
          glm::quat(channel->mRotationKeys[channel->mNumRotationKeys > 1 ? i : 0].mValue.w,
                    channel->mRotationKeys[channel->mNumRotationKeys > 1 ? i : 0].mValue.x,
                    channel->mRotationKeys[channel->mNumRotationKeys > 1 ? i : 0].mValue.y,
                    channel->mRotationKeys[channel->mNumRotationKeys > 1 ? i : 0].mValue.z);
        glm::vec3 scale =
          glm::make_vec3(&channel->mScalingKeys[channel->mNumScalingKeys > 1 ? i : 0].mValue.x);
        animation->keyframes[i].bones[j] =
          glm::scale(scale) * glm::translate(translation) * glm::mat4(rotation);
      }
    }

    // Pre-multiply with parents
    auto keyframes = animation->keyframes;
    for (uint32_t j = 0; j < anim->mNumChannels; ++j) {
      auto channel = anim->mChannels[j];
      for (uint32_t i = 0; i < animation->frame_count; i++) {
        auto model = keyframes[i].bones[j];
        for (auto node = node_map[channel->mNodeName.C_Str()]->mParent; node != nullptr;
             node = node->mParent) {
          // If the parent is not animated, use the nodes
          auto found = animation_node_map.find(node->mName.C_Str());
          if (found == animation_node_map.end()) {
            glm::mat4 transformation = glm::transpose(glm::make_mat4(node->mTransformation[0]));
            model = transformation * model;
          } else {
            glm::mat4 transformation = animation->keyframes[i].bones[found->second];
            model = transformation * model;
          }
        }
        keyframes[i].bones[j] = model;
      }
    }
    animation->keyframes = keyframes;

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
    case FileType::ANM: {
      auto animation = load_anm_file(path);
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
    case FileType::BVH:
      return load_assimp_file(path, true);
    case FileType::FBX:
    case FileType::Unknown:
    default:
      return load_assimp_file(path, false);
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
    bvhLoader.load(&bvh, path.filename().string());
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

std::vector<std::pair<entt::hashed_string, ResourceManager::Type>>
ResourceManager::load_assimp_file(const std::filesystem::path& path, bool skip_meshes)
{
  auto scene = aiImportFile(path.string().c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
  std::vector<std::pair<entt::hashed_string, ResourceManager::Type>> result;
  for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
    std::string name = path.filename().string() + ":" + scene->mAnimations[i]->mName.C_Str();
    auto id = entt::hashed_string{ name.c_str() };
    animation_cache_.load<Loader::Animation>(id, name, scene->mAnimations[i], scene->mRootNode);
  }
  if (!skip_meshes) {
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
      std::string name = path.filename().string() + ":" + scene->mMeshes[i]->mName.C_Str();
      auto id = entt::hashed_string{ name.c_str() };
      mesh_cache_.load<Loader::Mesh>(id, name, scene->mMeshes[i], scene->mRootNode);
    }
  }
  aiReleaseImport(scene);
  return {};
}
