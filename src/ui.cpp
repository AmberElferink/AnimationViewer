#include "ui.h"

#include <map>

#include <SDL_video.h>
#include <imgui.h>

#include <examples/imgui_impl_opengl3.h>
#include <examples/imgui_impl_sdl.h>

#include "renderer.h"

using namespace AnimationViewer;

std::unique_ptr<Ui>
Ui::create(SDL_Window* const window)
{
  if (!IMGUI_CHECKVERSION()) {
    return nullptr;
  }
  auto context = ImGui::CreateContext();
  if (context == nullptr) {
    return nullptr;
  }

  // Setup Platform/Renderer bindings
  if (!ImGui_ImplSDL2_InitForOpenGL(window, nullptr)) {
    ImGui::DestroyContext(context);
    return nullptr;
  }
  if (!ImGui_ImplOpenGL3_Init()) {
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(context);
    return nullptr;
  }

  return std::unique_ptr<Ui>(new Ui(window, context));
}

Ui::Ui(SDL_Window* const window, ImGuiContext* const context)
  : window_(window)
  , context_(context)
  , show_stats_(false)
{}

Ui::~Ui()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext(context_);
}

void
Ui::run(Scene& scene,
        const std::vector<std::pair<std::string, float>>& renderer_metrics,
        std::chrono::microseconds& dt)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window_);
  ImGui::NewFrame();

  ImGui::BeginMainMenuBar();
  std::map<std::string, std::vector<float>> graphs_map;
  for (auto [format, value] : renderer_metrics) {
    if (format.find("[GRAPH") != std::string::npos) {
      if (show_stats_) {
        auto title_start = format.find("] ");
        if (title_start != std::string::npos) {
          auto name = format.substr(title_start + 2);
          if (graphs_map.count(name) > 0) {
            graphs_map[name].push_back(value);
          } else {
            graphs_map.emplace(name, std::vector{ value });
          }
        }
      }
      continue;
    }
    ImGui::Text(format.c_str(), value);
  }
  ImGui::EndMainMenuBar();

  if (!graphs_map.empty()) {
    if (ImGui::Begin("Stats"), &show_stats_) {
      for (auto [title, values] : graphs_map) {
        ImGui::PlotHistogram(title.c_str(), values.data(), (int)values.size());
      }
    }
    ImGui::End();
  }

  if (ImGui::Begin("Configuration ")) {
    ImGui::Text("%.2f fps %.2f ms", 1e6f / dt.count(), dt.count() / 1000.0f);
    ImGui::Checkbox("Show stats", &show_stats_);
  }
  ImGui::End();

  // Rendering
  ImGui::Render();
}

void
Ui::draw() const
{
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
Ui::process_event(const SDL_Event& event)
{
  ImGui_ImplSDL2_ProcessEvent(&event);
}
