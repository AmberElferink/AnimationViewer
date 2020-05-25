#include "scene.h"
#include "resource.h"

#include <string>

#include <glm/trigonometric.hpp>

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


//TODO: Make an animation manager to put the code below. 
// It should have an update function with timestamps as well, called from game.cpp
// It should have an accessible ResourceManager (the one below does not work), find another way.
void Scene::add_mesh(const entt::hashed_string& id, const glm::ivec2& screen_space_position, ResourceManager& resource_manager)
{
  meshes_entity_.ids.push_back(id);

  const auto& mesh = resource_manager.mesh_cache().handle(id);

  for (int i = 0; i < mesh->bones.size(); i++)
  {
      const bone_t& bone = mesh->bones[i];
      glm::mat4 trans = glm::translate(glm::mat4(), { bone.position.x, bone.position.y, bone.position.z });
      glm::mat4 rot = glm::mat4(bone.orientation);
      glm::mat4 trans_rot = rot * trans;
      // mesh entity data add
      meshes_entity_.bone_trans_rots.push_back(rot);
  }
  int w = 0;
}

void
Scene::run(ResourceManager& resource_manager)
{
    //calc_bone_trans_rots(resource_manager);
}

const Camera&
Scene::active_camera() const
{
  if (cameras_.empty() || active_camera_ > cameras_.size()) {
    return default_camera_;
  }
  return cameras_[active_camera_];
}

const MeshEntityData& Scene::meshes() const
{
  return meshes_entity_;                                                                                                   
}
