#include "window.h"
#include <SDL.h>
#include <SDL_video.h>

using namespace AnimationViewer;

std::unique_ptr<Window>
Window::create(const std::string& name, uint16_t width, uint16_t height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    return nullptr;
  }
  auto handle = SDL_CreateWindow(name.c_str(),
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 width,
                                 height,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (handle == nullptr) {
    return nullptr;
  }
  return std::unique_ptr<Window>(new Window(handle));
}

Window::Window(SDL_Window* const handle)
  : handle_(handle)
{}

Window::~Window()
{

  SDL_DestroyWindow(handle_);
  SDL_Quit();
}

SDL_Window*
Window::get_native_handle() const
{
  return handle_;
}

void
Window::swap() const
{
  SDL_GL_SwapWindow(handle_);
}

void
Window::get_dimensions(uint16_t& width, uint16_t& height)
{
  int w, h;
#if _WIN32
  w = SDL_GetWindowSurface(handle)->w;
  h = SDL_GetWindowSurface(handle)->h;
#else
  SDL_GetWindowSize(handle_, &w, &h);
#endif
  width = static_cast<uint16_t>(w);
  height = static_cast<uint16_t>(h);
}
