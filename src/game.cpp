#include "game.h"

#include "input.h"
#include "renderer.h"
#include "resource.h"
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
  auto window = Window::create(app_name, width, height);
  if (!window) {
    return nullptr;
  }
  auto input = Input::create();
  if (!input) {
    return nullptr;
  }
  auto renderer = Renderer::create(window->get_native_handle());
  if (!renderer) {
    return nullptr;
  }
  auto ui = Ui::create(window->get_native_handle(), renderer->context_handle());
  if (!ui) {
    return nullptr;
  }
  auto scene = Scene::create();
  if (!scene) {
    return nullptr;
  }
  auto resource_manager = ResourceManager::create();
  if (!resource_manager) {
    return nullptr;
  }

  return std::unique_ptr<Game>(new Game(std::move(window),
                                        std::move(input),
                                        std::move(renderer),
                                        std::move(ui),
                                        std::move(scene),
                                        std::move(resource_manager)));
}

Game::Game(std::unique_ptr<Window>&& window,
           std::unique_ptr<Input>&& input,
           std::unique_ptr<Renderer>&& renderer,
           std::unique_ptr<Ui>&& ui,
           std::unique_ptr<Scene>&& scene,
           std::unique_ptr<ResourceManager>&& resource_manager)
  : window_(std::move(window))
  , input_(std::move(input))
  , renderer_(std::move(renderer))
  , ui_(std::move(ui))
  , scene_(std::move(scene))
  , resource_manager_(std::move(resource_manager))
{}

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
  scene_->set_default_camera_aspect(static_cast<float>(width) / height);
  input_->run(*window_, *ui_, *scene_, *resource_manager_, delta_time);
  ui_->run(*window_, *scene_, *resource_manager_, renderer_metrics_, delta_time);
  resource_manager_->upload_dirty_buffers(*renderer_);
  renderer_->render(*scene_, *resource_manager_, *ui_, delta_time);
  scene_->update(*resource_manager_, delta_time);
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
  /*resource_manager_->load_file("/home/sandy/Downloads/edited_bvh_Take_001_Take_001.bvh");
  auto resources = resource_manager_->load_file("/home/sandy/Downloads/cesiumman3.fbx");*/
  resource_manager_->load_file("D:\\Thijs\\Downloads\\cmuconvert-mb2-01-09\\01\\test.bvh");
  auto resources = resource_manager_->load_file("D:\\Thijs\\Downloads\\cmuconvert-mb2-01-09\\01\\cesiumman3.fbx");
  for (auto& [str, type] : resources) {
      scene_->add_mesh(str, std::nullopt, *resource_manager_);
  }

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
