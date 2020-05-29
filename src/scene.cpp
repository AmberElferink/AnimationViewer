#include "scene.h"

#include <iostream>
#include <string>

#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>

#include "resource.h"

using namespace AnimationViewer;

std::unique_ptr<Scene>
Scene::create()
{
  return std::unique_ptr<Scene>(new Scene());
}

Scene::Scene()
  : default_camera_(glm::vec3{ 0.0f, 1.0f, 3.0f }, 0, 0, glm::radians(80.0f))
{}
Scene::~Scene() = default;

void
Scene::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  if (cameras_.empty() || active_camera_ > cameras_.size()) {
    default_camera_.process_event(event, dt);
  } else {
    cameras_[active_camera_].process_event(event, dt);
  }
}

// TODO: Make an animation manager to put the code below.
// It should have an update function with timestamps as well, called from game.cpp
// It should have an accessible ResourceManager (the one below does not work), find another way.
void
Scene::add_mesh(ENTT_ID_TYPE id,
                const glm::ivec2& screen_space_position,
                ResourceManager& resource_manager)
{
  const auto& mesh = resource_manager.mesh_cache().handle(id);

  std::vector<glm::mat4> armature;
  armature.reserve(mesh->bones.size());
  for (size_t i = 0; i < mesh->bones.size(); i++) {
    const bone_t& bone = mesh->bones[i];
    glm::mat4 rot = glm::mat4(bone.orientation);
    glm::mat4 trans =
      glm::translate(glm::mat4(1.0f), { bone.position.x, bone.position.y, bone.position.z });
    glm::mat4 trans_rot = trans * rot;

    int parent_id = bone.parent;
    while (parent_id != -1) {
      const bone_t& parent_bone = mesh->bones[parent_id];

      glm::mat4 parent_trans =
        glm::translate(glm::mat4(1.0f),
                       { parent_bone.position.x, parent_bone.position.y, parent_bone.position.z });
      glm::mat4 parent_rot = glm::mat4(parent_bone.orientation);
      glm::mat4 parent_trans_rot = parent_trans * parent_rot;
      trans_rot = parent_trans_rot * trans_rot;

      parent_id = parent_bone.parent;
    }

    // mesh entity data add
    armature.push_back(trans_rot);
  }

  // construct a naked entity with no components (like a GameObject in Unity) and return its
  // identifier
  auto entity = registry_.create();
  // Add a mesh component to entity
  registry_.emplace<Components::Mesh>(entity, id);
  // Add an armature component to entity
  registry_.emplace<Components::Armature>(entity, armature);
}

void
Scene::run(ResourceManager& resource_manager)
{
  // calc_bone_trans_rots(resource_manager);
}

const Camera&
Scene::active_camera() const
{
  if (cameras_.empty() || active_camera_ > cameras_.size()) {
    return default_camera_;
  }
  return cameras_[active_camera_];
}

entt::registry&
Scene::registry()
{
  return registry_;
}

const entt::registry&
Scene::registry() const
{
  return registry_;
}
