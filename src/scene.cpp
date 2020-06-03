#include "scene.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <SDL_events.h>
#include <entt/entt.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include "resource.h"

using namespace AnimationViewer;

std::unique_ptr<Scene>
Scene::create()
{
  return std::unique_ptr<Scene>(new Scene());
}

Scene::Scene()
{
  auto camera_entity = registry_.create();
  registry_.emplace<Components::Camera>(camera_entity, Scene::default_camera());
  registry_.emplace<Components::Transform>(camera_entity, glm::vec3(0), glm::vec3(0), glm::vec3(1));
}
Scene::~Scene() = default;

void
Scene::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  const auto cameras = registry_.view<Components::Camera, Components::Transform>();
  if (!cameras.empty()) {
    auto camera_entity = cameras.front();
    auto& transform = registry_.get<Components::Transform>(camera_entity);

    float speed = 0.00001f * dt.count();
    switch (event.type) {
      case SDL_JOYAXISMOTION: {
        speed *= 0.0025f;
        switch (event.jaxis.axis) {
          // Translate x
          case 0:
            transform.position +=
              static_cast<float>(event.jaxis.value * speed * M_PI) *
              glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(1, 0, 0, 0));
            break;
            // Translate y
          case 1:
            transform.position +=
              static_cast<float>(event.jaxis.value * speed * M_PI) *
              glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(0, 1, 0, 0));
            break;
            // Translate z
          case 2:
            transform.position +=
              static_cast<float>(event.jaxis.value * speed * M_PI) *
              glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(0, 0, -1, 0));
            break;
            // Rotate x
          case 3:
            transform.orientation =
              transform.orientation * glm::angleAxis(speed, glm::vec3(-1.0f, 0.0f, 0.0f));
            break;
            // Rotate y
          case 4:
            transform.orientation =
              glm::angleAxis(speed, glm::vec3(0.0f, -1.0f, 0.0f)) * transform.orientation;
            break;
        }
      } break;
      case SDL_KEYDOWN: {
        if (event.key.keysym.mod & KMOD_SHIFT) {
          speed *= 5.0f;
        } else if (event.key.keysym.mod & KMOD_CTRL) {
          speed *= 0.2f;
        }
        switch (event.key.keysym.sym) {
          case SDLK_w: {
            transform.position +=
              speed * glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(0, 0, -1, 0));
          } break;
          case SDLK_s: {
            transform.position +=
              speed * glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(0, 0, 1, 0));
          } break;
          case SDLK_e:
            transform.position.y += speed;
            break;
          case SDLK_q:
            transform.position.y -= speed;
            break;
          case SDLK_a: {
            transform.position +=
              speed * glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(-1, 0, 0, 0));
          } break;
          case SDLK_d: {
            transform.position +=
              speed * glm::vec3(glm::toMat4(transform.orientation) * glm::vec4(1, 0, 0, 0));
          } break;
          case SDLK_UP: {
            transform.orientation =
              transform.orientation * glm::angleAxis(speed, glm::vec3(1.0f, 0.0f, 0.0f));
          } break;
          case SDLK_DOWN: {
            transform.orientation =
              transform.orientation * glm::angleAxis(speed, glm::vec3(-1.0f, 0.0f, 0.0f));
          } break;

          case SDLK_LEFT: {
            transform.orientation =
              glm::angleAxis(speed, glm::vec3(0.0f, 1.0f, 0.0f)) * transform.orientation;
          } break;
          case SDLK_RIGHT: {
            transform.orientation =
              glm::angleAxis(speed, glm::vec3(0.0f, -1.0f, 0.0f)) * transform.orientation;
          } break;
        }
      }
    }
  } else {
    assert(false);
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

Components::Camera
Scene::default_camera()
{
  return Components::Camera{
    .fov_y = glm::radians(80.0f),
    .near = 1.0f,
    .far = 1000.0f,
  };
}
