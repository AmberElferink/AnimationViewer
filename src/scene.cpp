#include "scene.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <SDL_events.h>
#include <entt/entt.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
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

  auto sky_entity = registry_.create();
  registry_.emplace<Components::Sky>(sky_entity, glm::vec3(0, 1, 0));
}
Scene::~Scene() = default;

void
Scene::set_default_camera_aspect(float aspect)
{
  const auto cameras = registry_.view<const Components::Camera, const Components::Transform>();
  if (!cameras.empty()) {
    auto& camera = registry_.get<Components::Camera>(cameras.front());
    camera.aspect = aspect;
  }
}

void
Scene::update(ResourceManager& resource_manager, std::chrono::microseconds& dt)
{
  registry_.view<Components::Animation>().each(
    [&resource_manager, &dt](auto entity, Components::Animation& animation) {
      if (!animation.animating) {
        return;
      }

      const auto& current_animation = resource_manager.animation_cache().handle(animation.id);
      animation.current_frame = animation.current_time * current_animation->frame_rate;
      if (animation.current_frame > current_animation->frame_count - 1) {
        if (animation.loop) {
          animation.current_frame = 0;
          animation.current_time = 0;
        } else {
          animation.current_frame = current_animation->frame_count - 1;
        }
        animation.animating = animation.loop;
      }

      animation.current_time += dt.count();
    });

  registry_.view<Components::MotionCaptureAnimation>().each(
    [&resource_manager, &dt](auto entity, Components::MotionCaptureAnimation& animation) {
      if (!animation.animating) {
        return;
      }
      const auto& current_animation = resource_manager.motion_capture_cache().handle(animation.id);
      // convert frame rate to microseconds
      animation.current_frame =
        static_cast<float>(animation.current_time) * current_animation->frame_rate * 1e-6f;
      auto frame_count = current_animation->frame_points.size() / current_animation->point_count;
      if (animation.current_frame > frame_count - 1) {
        if (animation.loop) {
          animation.current_frame = 0;
          animation.current_time = 0;
        } else {
          animation.current_frame = frame_count - 1;
        }
        animation.animating = animation.loop;
      }

      animation.current_time += dt.count();
    });
}

void
Scene::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  const auto cameras = registry_.view<Components::Camera, Components::Transform>();
  if (!cameras.empty()) {
    auto camera_entity = cameras.front();
    auto& transform = registry_.get<Components::Transform>(camera_entity);

    float speed = 0.0003f * dt.count();
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
                const std::optional<glm::vec2>& screen_space_position,
                const ResourceManager& resource_manager)
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

      glm::mat4 parent_trans = glm::translate(parent_bone.position);
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
  // Add a transform component to entity
  auto& transform = registry_.emplace<Components::Transform>(entity);
  if (screen_space_position) {
    const auto cameras = registry_.view<const Components::Camera, const Components::Transform>();
    if (!cameras.empty()) {
      auto& camera = registry_.get<Components::Camera>(cameras.front());
      auto& camera_transform = registry_.get<Components::Transform>(cameras.front());
      transform.position = camera_transform.position;
      auto xy = std::tan(camera.fov_y * 0.5f) * (2.0f * *screen_space_position - glm::vec2(1));
      xy *= glm::vec2(camera.aspect, -1.0f);
      transform.position = glm::vec3(xy, -1.0f);
      transform.position *= 10.0f;
      transform.orientation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
      assert(false);
    }
  }

  if (mesh->default_matrix.has_value()) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(*mesh->default_matrix,
                   transform.scale,
                   transform.orientation,
                   transform.position,
                   skew,
                   perspective);
  }
  // Add a mesh component to entity
  registry_.emplace<Components::Mesh>(entity, id);
  if (!armature.empty()) {
    // Add an armature component to entity
    registry_.emplace<Components::Armature>(entity, armature);
  }
}

bool
Scene::attach_animation(const entt::entity& entity,
                        ENTT_ID_TYPE id,
                        const ResourceManager& resource_manager)
{
  const auto animation_resource = resource_manager.animation_cache().handle(id);

  auto& armature = registry_.get<Components::Armature>(entity);
  if (armature.joints.size() != animation_resource->keyframes[0].bones.size()) {
    auto& animation = registry_.emplace<Components::Animation>(entity, id);
    animation.loop = true;
    animation.animating = true;
    auto& mesh = registry_.get<Components::Mesh>(entity);
    const auto& mesh_resource = resource_manager.mesh_cache().handle(mesh.id);

    std::unordered_map<std::string, uint32_t> bone_map;
    for (uint32_t i = 0; i < mesh_resource->bones.size(); ++i) {
      bone_map.emplace(mesh_resource->bones[i].name, i);
    }

    animation.transformed_matrices.resize(animation_resource->keyframes.size());
    for (uint32_t i = 0; i < animation_resource->keyframes.size(); ++i) {
      animation.transformed_matrices[i].resize(armature.joints.size());
      for (uint32_t j = 0; j < armature.joints.size(); ++j) {
        animation.transformed_matrices[i][j] = armature.joints[j];
      }
      for (uint32_t j = 0; j < animation_resource->joint_names.size(); ++j) {
        auto name = animation_resource->joint_names[j];
        auto found = bone_map.find(name);
        if (found != bone_map.end()) {
          printf("%s\n", name.c_str());
          animation.transformed_matrices[i][found->second] =
            animation_resource->keyframes[i].bones[j];
        }
      }
    }

  } else {

    auto& animation = registry_.emplace<Components::Animation>(entity, id);
    animation.loop = true;
    animation.animating = true;
    auto& mesh = registry_.get<Components::Mesh>(entity);
    const auto& mesh_resource = resource_manager.mesh_cache().handle(mesh.id);

    animation.transformed_matrices.reserve(animation_resource->keyframes.size());
    for (uint32_t i = 0; i < animation_resource->keyframes.size(); i++) {
      animation.transformed_matrices.push_back(std::vector<glm::mat4>());
      animation.transformed_matrices[i].reserve(animation_resource->keyframes[i].bones.size());
      for (uint32_t j = 0; j < animation_resource->keyframes[i].bones.size(); j++) {
        int parent_id = mesh_resource->bones[j].parent;

        auto transformed_mat = animation_resource->keyframes[i].bones[j];

        while (parent_id != -1) {
          const bone_t& parent_bone = mesh_resource->bones[parent_id];
          const glm::mat4 parent_joint = animation_resource->keyframes[i].bones[parent_id];

          transformed_mat = parent_joint * transformed_mat;

          parent_id = parent_bone.parent;
        }

        animation.transformed_matrices[i].push_back(transformed_mat);
      }
    }
  }

  // Can't have both animation and mocap animation
  if (registry_.has<Components::MotionCaptureAnimation>(entity)) {
    registry_.remove<Components::MotionCaptureAnimation>(entity);
  }

  return true;
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
    .aspect = 1.0f,
    .near = 0.01f,
    .far = 1000.0f,
  };
}
