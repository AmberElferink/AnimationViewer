#include "camera.h"
#include <SDL_events.h>

#include <glm/gtx/euler_angles.hpp>

using namespace AnimationViewer;

Camera::Camera(const glm::vec3& origin, float yaw, float pitch, float fov_y) noexcept
  : origin_(origin)
  , yaw_(yaw)
  , pitch_(pitch)
  , fov_y_(fov_y)
  , near_(0.001f)
  , far_(1000.0f)
{}

void
Camera::process_event(const SDL_Event& event, std::chrono::microseconds& dt)
{
  float speed = 0.00001f * dt.count();
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
          origin_ += speed * glm::vec3(glm::eulerAngleXY(pitch_, -yaw_) * glm::vec4(0, 0, -1, 0));
        } break;
        case SDLK_s: {
          origin_ += speed * glm::vec3(glm::eulerAngleXY(pitch_, -yaw_) * glm::vec4(0, 0, 1, 0));
        } break;
        case SDLK_e:
          origin_.y += speed;
          break;
        case SDLK_q:
          origin_.y -= speed;
          break;
        case SDLK_a: {
          origin_ += speed * glm::vec3(glm::eulerAngleXY(pitch_, -yaw_) * glm::vec4(-1, 0, 0, 0));
        } break;
        case SDLK_d: {
          origin_ += speed * glm::vec3(glm::eulerAngleXY(pitch_, -yaw_) * glm::vec4(1, 0, 0, 0));
        } break;
        case SDLK_UP:
          pitch_ += speed;
          break;
        case SDLK_DOWN:
          pitch_ -= speed;
          break;

        case SDLK_LEFT: {
          yaw_ -= speed;
        } break;
        case SDLK_RIGHT: {
          yaw_ += speed;
        } break;
      }
    }
  }
}

float
Camera::fov_y() const
{
  return fov_y_;
}

glm::vec3
Camera::origin() const
{
  return origin_;
}

glm::mat4
Camera::perspective(float aspect) const
{
  return glm::perspective(fov_y_, aspect, near_, far_);
}

glm::mat4
Camera::matrix() const
{
  return glm::transpose(glm::translate(glm::eulerAngleXY(-pitch_, yaw_), -origin_));
}
