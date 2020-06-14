#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <entt/entity/registry.hpp>

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
  void AcceptAnimation(Scene& scene, const entt::entity& entity, const ResourceManager& resource_manager);
  void draw() const;
  bool process_event(const Window& window, const SDL_Event& event);
  bool has_mouse() const;
  bool mouse_over_scene_window() const;

protected:
  Ui(SDL_Window* const window, ImGuiContext* const context);

private:
  const std::unique_ptr<ImGuiContext, ImGuiDestroyer> context_;
  bool show_statistics_;
  bool show_assets_;
  bool show_scene_;
  bool show_components_;
  bool scene_window_hovered_;
};
} // namespace AnimationViewer
