#include "camera.h"
#include <SDL_events.h>

#define _USE_MATH_DEFINES
#include <glm/geometric.hpp>
#include <math.h>

using namespace AnimationViewer;

Camera::Camera(const glm::mat4& matrix, float y_fov_deg) noexcept
  : matrix_(matrix)
  , y_fov_(y_fov_deg)
{}

void
Camera::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  float speed = 0.000001f * dt.count();
  switch (event.type) {
    case SDL_JOYAXISMOTION: {
      speed *= 0.0025f;
      switch (event.jaxis.axis) {
        // Translate x
        case 0: {
        } break;
        // Translate y
        case 1: {
        } break;
        // Translate z
        case 2: {
        } break;
        // Rotate x
        case 3: {
        } break;
        // Rotate y
        case 4: {
        } break;
      }
    } break;
    case SDL_KEYDOWN: {
      if (event.key.keysym.mod & KMOD_SHIFT) {
        speed *= 5.0f;
      } else if (event.key.keysym.mod & KMOD_CTRL) {
        speed *= 0.2f;
      }
      switch (event.key.keysym.sym) {
        case SDLK_w: {
        } break;
        case SDLK_s: {
        } break;
        case SDLK_e:
          break;
        case SDLK_q:
          break;
        case SDLK_a: {
        } break;
        case SDLK_d: {
        } break;
        case SDLK_UP:
          break;
        case SDLK_DOWN:
          break;

        case SDLK_LEFT: {
        } break;
        case SDLK_RIGHT: {
        } break;
      }
    }
  }
}
