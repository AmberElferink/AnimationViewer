#include "game.h"

#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "ui.h"
#include "window.h"

#if __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

using namespace AnimationViewer;
using namespace AnimationViewer::Graphics;

std::unique_ptr<Game>
Game::create(const std::string& app_name, uint16_t width, uint16_t height)
{
  return std::unique_ptr<Game>(new Game(app_name, width, height));
}

Game::Game(const std::string& app_name, uint16_t width, uint16_t height)
{
  window_ = Window::create(app_name, width, height);
  input_ = Input::create();
  renderer_ = Renderer::create(window_->get_native_handle());
  ui_ = Ui::create(window_->get_native_handle());
  scene_ = Scene::create();
}

Game::~Game() = default;

void
Game::clean_up()
{
  renderer_metrics_.clear();
  scene_.reset();
  ui_.reset();
  renderer_.reset();
  input_.reset();
  window_.reset();
}

bool
Game::main_loop()
{
  take_timestamp();
  auto delta_time = get_delta_time();
  uint16_t width, height;
  window_->get_dimensions(width, height);
  renderer_->set_back_buffer_size(width, height);
  input_->run(*ui_, *scene_, delta_time);
  ui_->run(*renderer_, renderer_metrics_, delta_time);
  renderer_->render_scene(*scene_);
  ui_->draw();
  window_->swap();

  return !input_->should_quit();
}

#if __EMSCRIPTEN__
void
em_main_loop_callback(void* arg)
{
  auto game = reinterpret_cast<Game*>(arg);
  if (!game->main_loop()) {
    game->clean_up();
    emscripten_cancel_main_loop();
  }
}
#endif

void
Game::run()
{
  frame_end_ = std::chrono::high_resolution_clock::now();
#if __EMSCRIPTEN__
  emscripten_set_main_loop_arg(em_main_loop_callback, this, 0, true);
#else
  while (main_loop()) {
  }
  clean_up();
#endif
}

void
Game::take_timestamp()
{
  frame_begin_ = frame_end_;
  frame_end_ = std::chrono::high_resolution_clock::now();
}
std::chrono::microseconds
Game::get_delta_time() const
{
  auto dt = frame_end_ - frame_begin_;
  return std::chrono::duration_cast<std::chrono::microseconds>(dt);
}
