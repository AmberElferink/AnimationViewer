#include "renderer.h"

#include <array>
#include <string_view>

#include <SDL_video.h>
#include <glad/glad.h>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>

#include <iostream>
#include <glm/gtx/string_cast.hpp>

#if __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "pipeline.h"
#include "resource.h"
#include "scene.h"
#include "ui.h"

#include "private_impl/graphics/buffer.h"
#include "private_impl/graphics/framebuffer.h"
#include "private_impl/graphics/indexed_mesh.h"
#include "private_impl/graphics/scoped_debug_group.h"
#include "private_impl/graphics/texture.h"

#include "private_impl/graphics/shaders/bridging_header.h"

#include "private_impl/graphics/shaders/disk_vert_glsl.h"
#include "private_impl/graphics/shaders/full_screen_vert_glsl.h"
#include "private_impl/graphics/shaders/mesh_frag_glsl.h"
#include "private_impl/graphics/shaders/mesh_vert_glsl.h"
#include "private_impl/graphics/shaders/rayleigh_sky_frag_glsl.h"
#include "private_impl/graphics/shaders/wireframe_frag_glsl.h"

using namespace AnimationViewer;
using namespace AnimationViewer::Graphics;

namespace {
void GLAPIENTRY
MessageCallback([[maybe_unused]] GLenum source,
                GLenum type,
                [[maybe_unused]] GLuint id,
                GLenum severity,
                [[maybe_unused]] GLsizei length,
                const GLchar* message,
                [[maybe_unused]] const void* userParam)
{
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }
  static constexpr std::array<std::string_view, GL_DEBUG_TYPE_OTHER - GL_DEBUG_TYPE_ERROR + 1>
    type_look_up{ {
      "ERROR",
      "DEPRECATED_BEHAVIOR",
      "UNDEFINED_BEHAVIOR",
      "PORTABILITY",
      "PERFORMANCE",
      "OTHER",
    } };
  static constexpr std::array<std::string_view, GL_DEBUG_SEVERITY_LOW - GL_DEBUG_SEVERITY_HIGH + 1>
    severity_look_up{ {
      "HIGH",
      "MEDIUM",
      "LOW",
    } };
  auto type_string = (type >= GL_DEBUG_TYPE_ERROR && type <= GL_DEBUG_TYPE_OTHER)
                       ? type_look_up[type - GL_DEBUG_TYPE_ERROR]
                       : "UNKNOWN";
  auto severity_string =
    (severity >= GL_DEBUG_SEVERITY_HIGH && severity <= GL_DEBUG_SEVERITY_LOW)
      ? severity_look_up[severity - GL_DEBUG_SEVERITY_HIGH]
      : (severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "NOTIFICATION" : "UNKNOWN");
  fprintf(stderr,
          "GL CALLBACK: type = %s severity = %s, message = %s\n",
          type_string.data(),
          severity_string.data(),
          message);
}
} // namespace

std::unique_ptr<Renderer>
Renderer::create(SDL_Window* window)
{
  return std::unique_ptr<Renderer>(new Renderer(window));
}

Renderer::Renderer(SDL_Window* window)
  : width_(0)
  , height_(0)
{
  // Request opengl 3.2 context.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
#if __EMSCRIPTEN__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

  // Turn on double buffering with a 24bit Z buffer.
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  context_ = std::unique_ptr<void, SDLDestroyer>(SDL_GL_CreateContext(window));
  if (context_) {
    gladLoadGLES2Loader(SDL_GL_GetProcAddress);

#if !__EMSCRIPTEN__
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, this);
#endif
    back_buffer_ = Framebuffer::default_framebuffer();
    create_geometry();
    create_pipeline();

    glEnable(GL_CULL_FACE);
  }
}

void
Renderer::SDLDestroyer::operator()(SDL_GLContext context) const
{
  SDL_GL_DeleteContext(context);
}

Renderer::~Renderer() = default;

std::vector<glm::mat4>
get_interpolated_armature(const Scene& scene,
                          const entt::entity& entity,
                          const ResourceManager& resource_manager)
{
  const auto& armature = scene.registry().get<Components::Armature>(entity);

  if (scene.registry().has<Components::Animation>(entity)) {
    const auto& animation = scene.registry().get<Components::Animation>(entity);

    // If the animation is at the last keyframe, render the bones without linear
    // interpolation. Else we linearly interpolate the bones for smoother animation.
    auto& matrices = animation.transformed_matrices;
    if (!animation.loop && animation.current_frame == matrices.size() - 1) {
      return animation.transformed_matrices[animation.current_frame];
    } else {
      const auto& current_animation = resource_manager.animation_cache().handle(animation.id);
      auto current_frame_timestamp = animation.current_frame / current_animation->frame_time;
      auto next_frame_timestamp = (animation.current_frame + 1) / current_animation->frame_time;

      // Calculate the normalized interpolation factor using the current keyframe timestamp
      // and next keyframe timestamp as the min and max values.
      auto interpolation_factor = glm::clamp((animation.current_time - current_frame_timestamp) /
                                               (next_frame_timestamp - current_frame_timestamp),
                                             0.0f,
                                             1.0f);

      // Linearly interpolate each bone matrix
      std::vector<glm::mat4> result;
      result.reserve(matrices[animation.current_frame].size());
      for (uint32_t i = 0; i < matrices[animation.current_frame].size(); i++) {
        if (current_animation->is_relative) {
          auto tempMatrix = glm::translate(matrices[animation.current_frame][i], glm::vec3(armature.joints[i][3]) / 3.0f);
          auto absolute_joint = tempMatrix * armature.joints[i];
          //auto next_absolute_joint = armature.joints[i] * matrices[(animation.current_frame + 1) % matrices.size()][i];
          /*result.push_back(glm::mix(absolute_joint,
                            next_absolute_joint,
                            interpolation_factor));*/
          result.push_back(absolute_joint);
        }
        else {
          result.push_back(glm::mix(matrices[animation.current_frame][i],
                          matrices[(animation.current_frame + 1) % matrices.size()][i],
                          interpolation_factor));
        }
      }
      return result;
    }
  } else { // if there is no animation, load default bone mat
    return armature.joints;
  }
}

void
Renderer::render(const Scene& scene,
                 const ResourceManager& resource_manager,
                 const Ui& ui,
                 const std::chrono::microseconds& dt)
{
  static const glm::vec4 clear_color = { 1.0f, 1.0f, 0.0f, 1.0f };
  if (!context_) {
    return;
  }

  const auto cameras =
    scene.registry().view<const Components::Camera, const Components::Transform>();
  Components::Camera camera = Scene::default_camera();
  glm::mat4 view_matrix(1.0f);
  glm::mat4 perspective_matrix = glm::perspective(glm::radians(60.f), 1.0f, 0.001f, 1000.0f);
  if (!cameras.empty()) {
    auto camera_entity = cameras.front();
    camera = scene.registry().get<Components::Camera>(camera_entity);
    auto transform = scene.registry().get<Components::Transform>(camera_entity);
    view_matrix =
      glm::translate(glm::transpose(glm::toMat4(transform.orientation)), -transform.position);
    perspective_matrix = glm::perspective(camera.fov_y, camera.aspect, camera.near, camera.far);
  } else {
    assert(false);
  }

  const auto skies = scene.registry().view<const Components::Sky>();
  vec3 direction_to_sun = glm::vec3(0, 1, 0);
  if (!cameras.empty()) {
    auto sky = skies.get<const Components::Sky>(skies.front());
    direction_to_sun = sky.direction_to_sun;
  }

  // clearing screen with a color which should never be seen
  back_buffer_->clear({ clear_color }, { 1.0f });

  {
    ScopedDebugGroup group("Rayleigh Sky in Screen Space");
    sky_uniform_t sky_uniform{
      view_matrix, direction_to_sun, camera.fov_y, width_, height_,
    };
    rayleigh_sky_uniform_buffer_->upload(&sky_uniform, sizeof(sky_uniform));

    rayleigh_sky_pipeline_->bind();
    full_screen_quad_->bind();
    rayleigh_sky_uniform_buffer_->bind(0);
    full_screen_quad_->draw();
  }

  {
    ScopedDebugGroup group("Draw Meshes");
    mesh_pipeline_->bind();
    mesh_vertex_uniform_buffer_->bind(0);

    mesh_uniform_t mesh_vertex_uniform{
      perspective_matrix, view_matrix, glm::mat4(), glm::vec4(direction_to_sun, 0), {},
    };
    // Get a multi component view of all entities which have component Mesh and Armature
    auto view = scene.registry().view<const Components::Transform, const Components::Mesh>();
    for (const auto& entity : view) {
      // Get the transform component of the entity
      const auto& transform = view.get<const Components::Transform>(entity);
      // Get the mesh component of the entity
      const auto& mesh = view.get<const Components::Mesh>(entity);

      mesh_vertex_uniform.model_matrix = glm::translate(transform.position) *
                                         glm::toMat4(transform.orientation) *
                                         glm::scale(transform.scale);

      // Get the Armature component of the entity
      if (scene.registry().has<Components::Armature>(entity)) {
        std::vector<glm::mat4> bone_trans_rots =
          get_interpolated_armature(scene, entity, resource_manager);

        /*std::cout << "New Matrix" << std::endl;
        for (auto mat : bone_trans_rots) {
          std::cout << glm::to_string(mat) << std::endl;
        }*/

        memcpy(mesh_vertex_uniform.bone_trans_rots,
               bone_trans_rots.data(),
               bone_trans_rots.size() * sizeof(bone_trans_rots[0]));
      } else {
        for (uint32_t i = 0; i < sizeof(mesh_vertex_uniform.bone_trans_rots) /
                                   sizeof(mesh_vertex_uniform.bone_trans_rots[0]);
             ++i) {
          mesh_vertex_uniform.bone_trans_rots[i] = glm::mat4(1.0f);
        }
      }

      mesh_vertex_uniform_buffer_->upload(&mesh_vertex_uniform, sizeof(mesh_vertex_uniform));

      // Mesh
      const auto& res = resource_manager.mesh_cache().handle(mesh.id);
      assert(res->gpu_resource);
      res->gpu_resource->bind();
      res->gpu_resource->draw();
    }
  }

  if (ui.draw_nodes()) {
    ScopedDebugGroup group("Draw Armatures");
    auto view = scene.registry().view<const Components::Transform, const Components::Armature>();
    for (const auto& entity : view) {
      for (const auto& model : get_interpolated_armature(scene, entity, resource_manager)) {
        const auto& transform = view.get<const Components::Transform>(entity);

        auto model_parent = glm::translate(transform.position) *
                            glm::toMat4(transform.orientation) * glm::scale(transform.scale);

        joint_disk_uniform_buffer_->bind(0);

        joint_uniform_t joint_disk_uniform = {
          .vp = perspective_matrix * view_matrix,
          .model = model_parent * model,
          .color = ui.node_display_color(),
          .node_size = ui.node_display_size(),
        };
        joint_disk_uniform_buffer_->upload(&joint_disk_uniform, sizeof(joint_disk_uniform));

        joint_pipeline_->bind();
        disk_->bind();
        disk_->draw();
      }
    }
  }

  {
    ScopedDebugGroup group("Draw Mocap points");

    auto view = scene.registry().view<const Components::MotionCaptureAnimation>();
    for (const auto& entity : view) {
      const auto& mocap = scene.registry().get<Components::MotionCaptureAnimation>(entity);
      const auto& mocap_resource = resource_manager.motion_capture_cache().handle(mocap.id);

      for (uint32_t i = 0; i < mocap_resource->point_count; ++i) {
        auto point =
          mocap_resource->frame_points[mocap.current_frame * mocap_resource->point_count + i] *
          mocap.scale;

        auto model = glm::scale(glm::vec3(mocap.scale)) * glm::translate(point);

        joint_disk_uniform_buffer_->bind(0);

        joint_uniform_t joint_disk_uniform = {
          .vp = perspective_matrix * view_matrix,
          .model = model,
          .color = ui.node_display_color(),
          .node_size = mocap.node_size,
        };
        joint_disk_uniform_buffer_->upload(&joint_disk_uniform, sizeof(joint_disk_uniform));

        joint_pipeline_->bind();
        disk_->bind();
        disk_->draw();
      }
    }
  }

  ui.draw();
  glFinish();
}

void
Renderer::set_back_buffer_size(uint16_t w, uint16_t h)
{
  if (w != width_ || h != height_) {
    width_ = w;
    height_ = h;

    rebuild_back_buffers();
  }
}

void
Renderer::rebuild_back_buffers()
{
  glViewport(0, 0, width_, height_);
}

void
Renderer::create_geometry()
{
  full_screen_quad_ = IndexedMesh::create_full_screen_quad();
  disk_ = IndexedMesh::create_disk_3_fan(16, 1.0f);
}

std::unique_ptr<IndexedMesh>
Renderer::upload_mesh(const std::vector<vertex_t>& vertices, const std::vector<uint16_t>& indices)
{
  const std::vector<IndexedMesh::MeshAttributes> attributes = {
    IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Position
    IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Normal
    IndexedMesh::MeshAttributes{ GL_FLOAT, 1 }, // Bone Id
  };
  return IndexedMesh::create(attributes,
                             vertices.data(),
                             vertices.size() * sizeof(vertices[0]),
                             indices.data(),
                             indices.size(),
                             IndexedMesh::PrimitiveTopology::TriangleList);
}

void*
Renderer::context_handle()
{
  return context_.get();
}

void
Renderer::create_pipeline()
{
  // Sky pipeline
  {
    Pipeline::CreateInfo info{
      .vertex_shader_binary = full_screen_vert_glsl,
      .vertex_shader_size = sizeof(full_screen_vert_glsl) / sizeof(full_screen_vert_glsl[0]),
      .vertex_shader_entry_point = "main",
      .fragment_shader_binary = rayleigh_sky_frag_glsl,
      .fragment_shader_size = sizeof(rayleigh_sky_frag_glsl) / sizeof(rayleigh_sky_frag_glsl[0]),
      .fragment_shader_entry_point = "main",
      .winding_order = Pipeline::TriangleWindingOrder::CounterClockwise,
      .cull_mode = Pipeline::CullMode::Back,
      .depth_write = false,
      .depth_test = Pipeline::DepthTest::Less,
    };
    rayleigh_sky_pipeline_ = Pipeline::create(Pipeline::Type::RasterOpenGL, info);
    rayleigh_sky_uniform_buffer_ = Buffer::create(sizeof(sky_uniform_t));
    rayleigh_sky_uniform_buffer_->set_debug_name("rayleigh_sky_uniform_buffer_");
  }
  // Mesh
  {
    Pipeline::CreateInfo info{
      .vertex_shader_binary = mesh_vert_glsl,
      .vertex_shader_size = sizeof(mesh_vert_glsl) / sizeof(mesh_vert_glsl[0]),
      .vertex_shader_entry_point = "main",
      .fragment_shader_binary = mesh_frag_glsl,
      .fragment_shader_size = sizeof(mesh_frag_glsl) / sizeof(mesh_frag_glsl[0]),
      .fragment_shader_entry_point = "main",
      .winding_order = Pipeline::TriangleWindingOrder::CounterClockwise,
      .cull_mode = Pipeline::CullMode::Back,
      .depth_write = true,
      .depth_test = Pipeline::DepthTest::Less,
    };
    mesh_pipeline_ = Pipeline::create(Pipeline::Type::RasterOpenGL, info);
    mesh_vertex_uniform_buffer_ = Buffer::create(sizeof(mesh_uniform_t));
    mesh_vertex_uniform_buffer_->set_debug_name("mesh_uniform_buffer_");
  }
  // Joints
  {
    Pipeline::CreateInfo info{
      .vertex_shader_binary = disk_vert_glsl,
      .vertex_shader_size = sizeof(disk_vert_glsl) / sizeof(disk_vert_glsl[0]),
      .vertex_shader_entry_point = "main",
      .fragment_shader_binary = wireframe_frag_glsl,
      .fragment_shader_size = sizeof(wireframe_frag_glsl) / sizeof(wireframe_frag_glsl[0]),
      .fragment_shader_entry_point = "main",
      .winding_order = Pipeline::TriangleWindingOrder::CounterClockwise,
      .cull_mode = Pipeline::CullMode::None,
      .depth_write = false,
      .depth_test = Pipeline::DepthTest::Never,
      .blend = true,
    };
    joint_disk_uniform_buffer_ = Buffer::create(sizeof(joint_uniform_t));
    joint_disk_uniform_buffer_->set_debug_name("joint_vertex_uniform_buffer_");
    joint_pipeline_ = Pipeline::create(Pipeline::Type::RasterOpenGL, info);
  }
}
