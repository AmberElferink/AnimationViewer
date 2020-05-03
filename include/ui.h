#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct ImGuiContext;
struct SDL_Window;
union SDL_Event;

namespace AnimationViewer {
namespace Graphics {
class Renderer;
} // namespace Graphics
class ResourceManager;
class Scene;

class Ui
{
public:
  static std::unique_ptr<Ui> create(SDL_Window* const window);
  virtual ~Ui();
  void run(Scene& scene,
           ResourceManager& resource_manager,
           const std::vector<std::pair<std::string, float>>& renderer_metrics,
           std::chrono::microseconds& dt);
  void draw() const;
  void process_event(const SDL_Event& event);

protected:
  Ui(SDL_Window* const window, ImGuiContext* const context);

private:
  SDL_Window* const window_;
  ImGuiContext* const context_;
  bool show_stats_;
};
} // namespace AnimationViewer
