#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <entt/fwd.hpp>
#include <glm/vec4.hpp>

struct ImGuiContext;
struct SDL_Window;
union SDL_Event;

namespace AnimationViewer {
namespace Graphics {
class Renderer;
} // namespace Graphics
class ResourceManager;
class Scene;
class Window;

class Ui
{
  struct ImGuiDestroyer
  {
    void operator()(ImGuiContext* context) const;
  };

public:
  static std::unique_ptr<Ui> create(SDL_Window* const window, void* gl_context);
  virtual ~Ui();
  void run(const Window& window,
           Scene& scene,
           ResourceManager& resource_manager,
           const std::vector<std::pair<std::string, float>>& renderer_metrics,
           std::chrono::microseconds& dt);
  void entity_dnd_target(Scene& scene,
                         const entt::entity& entity,
                         const ResourceManager& resource_manager);
  bool entity_accept_animation(Scene& scene,
                               const entt::entity& entity,
                               const ResourceManager& resource_manager);
  bool entity_accept_mocap(Scene& scene,
                           const entt::entity& entity,
                           const ResourceManager& resource_manager);
  void draw() const;
  bool process_event(const Window& window, const SDL_Event& event);
  bool has_mouse() const;
  bool mouse_over_scene_window() const;
  bool draw_nodes() const;
  float node_display_size() const;
  glm::vec4 node_display_color() const;

protected:
  Ui(SDL_Window* const window, ImGuiContext* const context);

private:
  const std::unique_ptr<ImGuiContext, ImGuiDestroyer> context_;
  bool show_statistics_;
  bool show_assets_;
  bool show_scene_;
  bool show_components_;
  bool scene_window_hovered_;
  bool show_nodes_;
  float node_size_;
  glm::vec4 node_color_;
};
} // namespace AnimationViewer
