#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace AnimationViewer {
class Input;
class Scene;
class Ui;
class Window;

namespace Graphics {
class Renderer;
}

using Graphics::Renderer;

class Game
{
public:
  static std::unique_ptr<Game> create(const std::string& app_name, uint16_t width, uint16_t height);
  virtual ~Game();

  void clean_up();

  void run();
  bool main_loop();

protected:
  Game(std::unique_ptr<Window>&& window,
       std::unique_ptr<Input>&& input,
       std::unique_ptr<Renderer>&& renderer,
       std::unique_ptr<Ui>&& ui,
       std::unique_ptr<Scene>&& scene);

private:
  void take_timestamp();
  std::chrono::microseconds get_delta_time() const;
  std::unique_ptr<Window> window_;
  std::unique_ptr<Input> input_;
  std::unique_ptr<Renderer> renderer_;
  std::unique_ptr<Ui> ui_;
  std::unique_ptr<Scene> scene_;
  std::chrono::high_resolution_clock::time_point frame_begin_;
  std::chrono::high_resolution_clock::time_point frame_end_;
  std::vector<std::pair<std::string, float>> renderer_metrics_;
};
} // namespace AnimationViewer
