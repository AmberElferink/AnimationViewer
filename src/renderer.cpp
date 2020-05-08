#include "renderer.h"

#include <array>
#include <string_view>

#include <SDL_video.h>
#include <glad/glad.h>
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
    type_look_up{
      "ERROR", "DEPRECATED_BEHAVIOR", "UNDEFINED_BEHAVIOR", "PORTABILITY", "PERFORMANCE", "OTHER",
    };
  static constexpr std::array<std::string_view, GL_DEBUG_SEVERITY_LOW - GL_DEBUG_SEVERITY_HIGH + 1>
    severity_look_up{
      "HIGH",
      "MEDIUM",
      "LOW",
    };
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

  context_ = SDL_GL_CreateContext(window);
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

Renderer::~Renderer()
{
  SDL_GL_DeleteContext(context_);
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

  const auto& camera = scene.active_camera();
  {
    mat4 view_matrix = glm::transpose(camera.matrix());
    vec3 direction_to_sun = glm::vec3(0, 1, 0);
    sky_uniform_t sky_uniform{
      view_matrix, direction_to_sun, camera.fov_y(), width_, height_,
    };
    rayleigh_sky_uniform_buffer_->upload(&sky_uniform, sizeof(sky_uniform));

    mesh_uniform_t mesh_vertex_uniform{
      camera.perspective(static_cast<float>(width_) / height_),
      view_matrix,
      direction_to_sun,
    };
    mesh_vertex_uniform_buffer_->upload(&mesh_vertex_uniform, sizeof(mesh_vertex_uniform));
  }

  // clearing screen with a color which should never be seen
  back_buffer_->clear({ clear_color });

  {
    ScopedDebugGroup group("Rayleigh Sky in Screen Space");
    rayleigh_sky_pipeline_->bind();
    full_screen_quad_->bind();
    rayleigh_sky_uniform_buffer_->bind(0);
    full_screen_quad_->draw();
  }

  {
    ScopedDebugGroup group("Draw Meshes");
    mesh_pipeline_->bind();
    mesh_vertex_uniform_buffer_->bind(0);
    for (const auto& id: scene.meshes()) {
      const auto& res = resource_manager.mesh_cache().handle(id);
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
Renderer::upload_mesh(std::vector<float> vertices, std::vector<uint16_t> indices)
{
  const std::vector<IndexedMesh::MeshAttributes> attributes = {
    IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Position
    IndexedMesh::MeshAttributes{ GL_FLOAT, 3 }, // Normal
  };
  return IndexedMesh::create(attributes,
                             vertices.data(),
                             vertices.size() * sizeof(vertices[0]),
                             indices.data(),
                             indices.size());
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
    };
    mesh_pipeline_ = Pipeline::create(Pipeline::Type::RasterOpenGL, info);
    mesh_vertex_uniform_buffer_ = Buffer::create(sizeof(mesh_uniform_t));
    mesh_vertex_uniform_buffer_->set_debug_name("mesh_uniform_buffer_");
  }
}
