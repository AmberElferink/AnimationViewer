#include "renderer.h"

#include <array>
#include <string_view>

#include <SDL_video.h>
#include <glad/glad.h>

#if __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "pipeline.h"
#include "scene.h"
#include "ui.h"

#include "private_impl/graphics/framebuffer.h"
#include "private_impl/graphics/indexed_mesh.h"
#include "private_impl/graphics/texture.h"

#include "private_impl/graphics/shaders/bridging_header.h"

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
  }
}

Renderer::~Renderer()
{
  SDL_GL_DeleteContext(context_);
}

void
Renderer::render(const Scene& scene,
                 const Ui& ui,
                 const std::chrono::microseconds& dt)
{
  static const glm::vec4 clear_color = { 1.0f, 1.0f, 0.0f, 1.0f };
  if (!context_) {
    return;
  }

  // auto& cam = scene.get_camera();

  // clearing screen
  back_buffer_->clear({ clear_color });

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
{}

void
Renderer::create_pipeline()
{}
