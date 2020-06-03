#include "renderer.h"

#include <array>
#include <string_view>

#include <SDL_video.h>
#include <glad/glad.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>

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

#include "private_impl/graphics/shaders/full_screen_vert_glsl.h"
#include "private_impl/graphics/shaders/mesh_frag_glsl.h"
#include "private_impl/graphics/shaders/mesh_vert_glsl.h"
#include "private_impl/graphics/shaders/rayleigh_sky_frag_glsl.h"

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
  if (!cameras.empty()) {
    auto camera_entity = cameras.front();
    camera = scene.registry().get<Components::Camera>(camera_entity);
    auto transform = scene.registry().get<Components::Transform>(camera_entity);
    view_matrix =
      glm::translate(glm::transpose(glm::toMat4(transform.orientation)), -transform.position);
  } else {
    assert(false);
  }
  vec3 direction_to_sun = glm::vec3(0, 1, 0);

  // clearing screen with a color which should never be seen
  back_buffer_->clear({ clear_color }, { 1.0f });

  mesh_uniform_t mesh_vertex_uniform{
    glm::perspective(camera.fov_y, static_cast<float>(width_) / height_, camera.near, camera.far),
    view_matrix,
    glm::mat4(),
    glm::vec4(direction_to_sun, 0),
    // bone_trans_rots filled with memcpy
    {},
  };

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

    // Get a multi component view of all entities which have component Mesh and Armature
    auto view =
      scene.registry()
        .view<const Components::Transform, const Components::Mesh, const Components::Armature>();
    for (const auto& entity : view) {
      // Get the transform component of the entity
      const auto& transform = view.get<const Components::Transform>(entity);
      // Get the mesh component of the entity
      const auto& mesh = view.get<const Components::Mesh>(entity);
      // Get the Armature component of the entity
      const auto& armature = view.get<const Components::Armature>(entity);

      mesh_vertex_uniform.model_matrix = glm::translate(transform.position) *
                                         glm::toMat4(transform.orientation) *
                                         glm::scale(transform.scale);
      const std::vector<glm::mat4>& bone_trans_rots = armature.joints;
      memcpy(mesh_vertex_uniform.bone_trans_rots,
             bone_trans_rots.data(),
             bone_trans_rots.size() * sizeof(bone_trans_rots[0]));

      mesh_vertex_uniform_buffer_->upload(&mesh_vertex_uniform, sizeof(mesh_vertex_uniform));

      // Mesh
      const auto& res = resource_manager.mesh_cache().handle(mesh.id);
      assert(res->gpu_resource);
      res->gpu_resource->bind();
      res->gpu_resource->draw();
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
{}

void
Renderer::create_geometry()
{
  full_screen_quad_ = IndexedMesh::create_full_screen_quad();
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
                             indices.size());
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
      .depth_write = true,
      .depth_test = Pipeline::DepthTest::Less,
    };
    mesh_pipeline_ = Pipeline::create(Pipeline::Type::RasterOpenGL, info);
    mesh_vertex_uniform_buffer_ = Buffer::create(sizeof(mesh_uniform_t));
    mesh_vertex_uniform_buffer_->set_debug_name("mesh_uniform_buffer_");
  }
}
