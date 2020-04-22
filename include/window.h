#pragma once

#include <cstdint>
#include <memory>
#include <string>

struct SDL_Window;

namespace AnimationViewer {
class Window
{
public:
  static std::unique_ptr<Window> create(const std::string& name, uint16_t width, uint16_t height);

  virtual ~Window();
  SDL_Window* get_native_handle() const;
  void swap() const;
  void get_dimensions(uint16_t& width, uint16_t& height);

protected:
  explicit Window(SDL_Window* const handle);

private:
  SDL_Window* const handle_;
};
} // namespace AnimationViewer
