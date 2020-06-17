#include "input.h"

#include <filesystem>

#include <SDL.h>

#include "resource.h"
#include "scene.h"
#include "ui.h"
#include "window.h"

using AnimationViewer::Input;

#if USE_SPNAV
#include <spnav.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

extern "C"
{
  void EMSCRIPTEN_KEEPALIVE // Required to be export
  animation_viewer_ui_load_file_contents(Input* input,
                                         const char* file_name,
                                         const uint8_t* contents,
                                         size_t size_)
  {
    FILE* file = fopen(file_name, "wb");
    fwrite(contents, size_, size_, file);
    fclose(file);
    SDL_Event event = {};
    event.type = SDL_DROPFILE;
    event.drop.timestamp = SDL_GetTicks();
    event.drop.file = (char*)SDL_malloc(size_);
    memcpy(event.drop.file, file_name, size_);
    SDL_PushEvent(&event);
  }
}

void
animation_viewer_emscripten_install_drop_handler(Input& input)
{
  EM_ASM(
    {
      var canvas = document.getElementById('canvas');

      canvas.addEventListener(
        'dragover', function(ev) {
          ev.stopPropagation();
          ev.preventDefault();
          ev.dataTransfer.dropEffect = 'copy';
        });

      canvas.addEventListener(
        'drop', function(ev) {
          ev.stopPropagation();
          ev.preventDefault();
          var files = ev.dataTransfer.files;
          for (var i = 0, file; file = files[i]; i++) {
            var reader = new FileReader();
            reader.onload = (function(f) {
              var len = lengthBytesUTF8(f.name) + 1;
              var file_name = stackAlloc(len);
              stringToUTF8(f.name, file_name, len);
              return function(ev2)
              {
                var file_contents = _malloc(ev2.target.result.byteLength);
                var heap_bytes =
                  new Uint8Array(HEAPU8.buffer, file_contents, ev2.target.result.byteLength);
                heap_bytes.set(new Uint8Array(ev2.target.result));
                ccall('animation_viewer_ui_load_file_contents',
                      'v',
                      'isii',
                      [ $0, file_name, file_contents, ev2.target.result.byteLength ],
                      []);
                _free(file_contents);
              };
            })(file);
            reader.readAsArrayBuffer(file);
          }
        });
    },
    &input);
}

#endif

std::unique_ptr<Input>
Input::create()
{
  auto result = std::unique_ptr<Input>(new Input());
#ifdef EMSCRIPTEN
  animation_viewer_emscripten_install_drop_handler(*result);
#endif
  return result;
}

Input::Input()
  : quit_(false)
{
#if USE_SPNAV
  if (spnav_open() == -1) {
    std::fprintf(stderr, "failed to connect to the space navigator daemon\n");
  }
#endif
}

Input::~Input()
{
#if USE_SPNAV
  spnav_close();
#endif
}

void
Input::run(const Window& window,
           Ui& ui,
           Scene& scene,
           ResourceManager& resource_manager,
           std::chrono::microseconds& dt)
{
  SDL_Event event;

#if USE_SPNAV
  thread_local int last_x = 0;
  thread_local int last_y = 0;
  thread_local int last_z = 0;
  thread_local int last_rx = 0;
  thread_local int last_ry = 0;
  thread_local int last_rz = 0;
  spnav_event sev;

  while (spnav_poll_event(&sev)) {
    if (sev.type == SPNAV_EVENT_MOTION) {
      if (last_x != sev.motion.x) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 0;
        event.jaxis.value = static_cast<Sint16>(sev.motion.x);
        SDL_PushEvent(&event);
        last_x = sev.motion.x;
      }
      if (last_y != sev.motion.y) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 1;
        event.jaxis.value = static_cast<Sint16>(sev.motion.y);
        SDL_PushEvent(&event);
        last_y = sev.motion.y;
      }
      if (last_z != sev.motion.z) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 2;
        event.jaxis.value = static_cast<Sint16>(sev.motion.z);
        SDL_PushEvent(&event);
        last_z = sev.motion.z;
      }
      if (last_rx != sev.motion.rx) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 3;
        event.jaxis.value = static_cast<Sint16>(sev.motion.rx);
        SDL_PushEvent(&event);
        last_rx = sev.motion.rx;
      }
      if (last_ry != sev.motion.ry) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 4;
        event.jaxis.value = static_cast<Sint16>(sev.motion.ry);
        SDL_PushEvent(&event);
        last_ry = sev.motion.ry;
      }
      if (last_rz != sev.motion.rz) {
        event.jaxis.type = SDL_JOYAXISMOTION;
        event.jaxis.timestamp = SDL_GetTicks();
        event.jaxis.which = 0;
        event.jaxis.axis = 5;
        event.jaxis.value = static_cast<Sint16>(sev.motion.rz);
        SDL_PushEvent(&event);
        last_rz = sev.motion.rz;
      }
    } else if (sev.type == SPNAV_EVENT_BUTTON) {
      event.jbutton.type = sev.button.press ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
      event.jbutton.timestamp = SDL_GetTicks();
      event.jbutton.which = 0;
      event.jbutton.button = static_cast<Uint8>(sev.button.bnum);
      event.jbutton.state = sev.button.press ? SDL_PRESSED : SDL_RELEASED;
      SDL_PushEvent(&event);
    }
  }
#endif

  while (SDL_PollEvent(&event)) {
    if (!ui.process_event(window, event)) {
      quit_ = true;
    }
    scene.process_event(event, dt);
    switch (event.type) {
      case SDL_QUIT:
        quit_ = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit_ = true;
            break;
        }
        break;
      case SDL_DROPFILE: {
        const auto path = std::filesystem::path(event.drop.file);
        glm::ivec2 mouse_position;
        SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
        glm::u16vec2 resolution;
        window.get_dimensions(resolution.x, resolution.y);
        glm::vec2 screen_space_position =
          static_cast<glm::vec2>(mouse_position) / static_cast<glm::vec2>(resolution);
        auto resources = resource_manager.load_file(path);
        for (auto& [str, type] : resources) {
          if (type & ResourceManager::Type::Mesh) {
            if (!ui.has_mouse()) {
              scene.add_mesh(str, screen_space_position, resource_manager);
            } else if (ui.mouse_over_scene_window()) {
              scene.add_mesh(str, std::nullopt, resource_manager);
            }
          }
        }
#ifdef __EMSCRIPTEN__
        // Files are created at runtime in memory, free this memory after load
        // std::filesystem::remove(path);
#endif
      } break;
    }
  }
}

bool
Input::should_quit() const
{
  return quit_;
}
