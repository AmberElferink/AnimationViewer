#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace AnimationViewer {
class Scene;
} // namespace AnimationViewer

namespace AnimationViewer::Graphics {
struct Framebuffer;

class Renderer
{
public:
  virtual ~Renderer();
  void render_scene(const Scene& scene);
  void set_back_buffer_size(uint16_t width, uint16_t height);

  /// Factory function from which all types of renderers can be created
  static std::unique_ptr<Renderer> create(SDL_Window* window);

protected:
  explicit Renderer(SDL_Window* window);

private:
  void rebuild_back_buffers();
  void create_geometry();
  void create_pipeline();

  SDL_GLContext context_;
  uint16_t width_;
  uint16_t height_;
  std::unique_ptr<Framebuffer> back_buffer_;
};
} // namespace AnimationViewer::Graphics
