#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace AnimationViewer {
class ResourceManager;
class Scene;
class Ui;
struct vertex_t;
} // namespace AnimationViewer

namespace AnimationViewer::Graphics {
class Buffer;
struct Framebuffer;
struct IndexedMesh;
class Pipeline;

class Renderer
{
  struct SDLDestroyer
  {
    void operator()(SDL_GLContext context) const;
  };

public:
  /// Factory function from which all types of renderers can be created
  static std::unique_ptr<Renderer> create(SDL_Window* window);
  virtual ~Renderer();

  void render(const Scene& scene,
              const ResourceManager& resource_manager,
              const Ui& ui,
              const std::chrono::microseconds& dt);
  void set_back_buffer_size(uint16_t width, uint16_t height);
  std::unique_ptr<IndexedMesh> upload_mesh(const std::vector<vertex_t>& vertices, const std::vector<uint16_t>& indices);
  void* context_handle();

protected:
  explicit Renderer(SDL_Window* window);

private:
  void rebuild_back_buffers();
  void create_geometry();
  void create_pipeline();

  std::unique_ptr<void, SDLDestroyer> context_;
  uint16_t width_;
  uint16_t height_;
  std::unique_ptr<Framebuffer> back_buffer_;
  std::unique_ptr<IndexedMesh> full_screen_quad_;
  std::unique_ptr<IndexedMesh> disk_;
  std::unique_ptr<Buffer> rayleigh_sky_uniform_buffer_;
  std::unique_ptr<Pipeline> rayleigh_sky_pipeline_;
  std::unique_ptr<Buffer> mesh_vertex_uniform_buffer_;
  std::unique_ptr<Pipeline> mesh_pipeline_;
  std::unique_ptr<Pipeline> joint_pipeline_;
  std::unique_ptr<Buffer> joint_disk_uniform_buffer_;
};
} // namespace AnimationViewer::Graphics
