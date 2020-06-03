#include "ui.h"

#include <map>

#include <SDL_events.h>
#include <SDL_video.h>
#include <imGuIZMOquat.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <examples/imgui_impl_opengl3.h>
#include <examples/imgui_impl_sdl.h>

#include "resource.h"
#include "scene.h"
#include "window.h"

using AnimationViewer::Ui;

std::unique_ptr<Ui>
Ui::create(SDL_Window* const window, void* gl_context)
{
  if (!IMGUI_CHECKVERSION()) {
    return nullptr;
  }
  auto context = ImGui::CreateContext();
  if (context == nullptr) {
    return nullptr;
  }

  // Enable docking and multi viewport
  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // Setup Platform/Renderer bindings
  if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
    ImGui::DestroyContext(context);
    return nullptr;
  }
  if (!ImGui_ImplOpenGL3_Init()) {
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(context);
    return nullptr;
  }

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
  // identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  return std::unique_ptr<Ui>(new Ui(window, context));
}

Ui::Ui(SDL_Window* const window, ImGuiContext* const context)
  : context_(context)
  , show_statistics_(false)
  , show_assets_(true)
  , show_scene_(true)
  , show_components_(true)
  , scene_window_hovered_(false)
{}

void
Ui::ImGuiDestroyer::operator()(ImGuiContext* context) const
{
  ImGui::DestroyContext(context);
}

Ui::~Ui()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
}

void
Ui::run(const Window& window,
        Scene& scene,
        ResourceManager& resource_manager,
        const std::vector<std::pair<std::string, float>>& renderer_metrics,
        std::chrono::microseconds& dt)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window.get_native_handle());
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Statistics", nullptr, &show_statistics_);
    ImGui::MenuItem("Assets", nullptr, &show_assets_);
    ImGui::MenuItem("Scene", nullptr, &show_scene_);
    ImGui::MenuItem("Components", nullptr, &show_components_);
    ImGui::EndMenu();
  }
  char frame_timing[32];
  snprintf(frame_timing,
           sizeof(frame_timing),
           "%.2f fps %.2f ms",
           1e6f / dt.count(),
           dt.count() / 1000.0f);
  ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(frame_timing).x);
  ImGui::Text("%5.2f fps %2.2f ms", 1e6f / dt.count(), dt.count() / 1000.0f);
  std::map<std::string, std::vector<float>> graphs_map;
  for (auto [format, value] : renderer_metrics) {
    if (format.find("[GRAPH") != std::string::npos) {
      if (show_statistics_) {
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

  if (show_statistics_ && ImGui::Begin("Statistics", &show_statistics_)) {
    if (!graphs_map.empty()) {
      for (auto [title, values] : graphs_map) {
        ImGui::PlotHistogram(title.c_str(), values.data(), (int)values.size());
      }
    }
    ImGui::End();
  }

  ImGuiStyle& style = ImGui::GetStyle();
  auto title_bar_height = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
  auto viewport = ImGui::GetMainViewport();
  auto dock_padding = title_bar_height;

  if (show_assets_) {
    dock_padding += viewport->Size.y * 0.2f;
    auto height = viewport->Size.y * 0.2f;
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - height),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Assets", &show_assets_)) {
      ImGui::Columns(2);
      if (ImGui::TreeNodeEx("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
        resource_manager.mesh_cache().each([&resource_manager](const auto id) {
          ImGui::TreeNodeEx(resource_manager.mesh_cache().handle(id)->name.c_str(),
                            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Drag and drop on scene to add to scene.");
          }
          if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("DND_MESH", &id, sizeof(id));
            ImGui::Text("Mesh: %s", resource_manager.mesh_cache().handle(id)->name.c_str());
            ImGui::EndDragDropSource();
          }
        });
        ImGui::TreePop();
      }
      ImGui::NextColumn();
      if (ImGui::TreeNodeEx("Animations", ImGuiTreeNodeFlags_DefaultOpen)) {
        resource_manager.animation_cache().each([&resource_manager](const auto id) {
          ImGui::TreeNodeEx(resource_manager.animation_cache().handle(id)->name.c_str(),
                            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Drag and drop on entity to add animation component.");
          }
          if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("DND_ANIMATION", &id, sizeof(id));
            ImGui::Text("Animation: %s",
                        resource_manager.animation_cache().handle(id)->name.c_str());
            ImGui::EndDragDropSource();
          }
        });
        ImGui::TreePop();
      }
      ImGui::Columns(1);
      ImGui::End();
    }
  }

  static std::optional<entt::entity> selected_entity;

  scene_window_hovered_ = false;
  if (show_scene_) {
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + title_bar_height),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.2f, viewport->Size.y - dock_padding),
                             ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene", &show_scene_)) {
      // Allow dropping meshes into scene
      if (ImGui::BeginDragDropTarget()) {
        auto payload = ImGui::AcceptDragDropPayload("DND_MESH");
        if (payload != nullptr) {
          ENTT_ID_TYPE id;
          assert(payload->DataSize == sizeof(id));
          memcpy(&id, payload->Data, sizeof(id));
          scene.add_mesh(id, { -1, -1 }, resource_manager);
        }
        ImGui::EndDragDropTarget();
      }

      char entity_name[256];
      uint32_t i = 0;
      std::string component_type;
      scene.registry().each(
        [&i, &scene, &entity_name, &component_type](const entt::entity& entity) {
          if (scene.registry().has<Components::Camera>(entity)) {
            component_type = " (Camera)";
          } else if (scene.registry().has<Components::Sky>(entity)) {
            component_type = " (Sky)";
          } else if (scene.registry().has<Components::Mesh>(entity)) {
            component_type = " (Mesh)";
          }
          snprintf(entity_name, sizeof(entity_name), "Entity %d%s", ++i, component_type.c_str());
          if (ImGui::Selectable(entity_name, selected_entity == entity)) {
            selected_entity = entity;
          }
          // Allow dropping assets into component if an entity is selected
          if (ImGui::BeginDragDropTarget()) {
            auto payload = ImGui::AcceptDragDropPayload("DND_ANIMATION");
            if (payload != nullptr) {
              ENTT_ID_TYPE id;
              assert(payload->DataSize == sizeof(id));
              memcpy(&id, payload->Data, sizeof(id));
              scene.registry().emplace<Components::Animation>(entity, id, /*dummy=*/0u);
            }
            ImGui::EndDragDropTarget();
          }
        });
      scene_window_hovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
      ImGui::End();
    }
  }

  if (show_components_) {
    ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x + viewport->Size.x * 0.8f, viewport->Pos.y + title_bar_height),
      ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.2f, viewport->Size.y - dock_padding),
                             ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Components", &show_components_)) {
      if (selected_entity.has_value()) {
        auto& registry = scene.registry();
        // Allow dropping assets into component if an entity is selected
        if (ImGui::BeginDragDropTarget()) {
          auto payload = ImGui::AcceptDragDropPayload("DND_ANIMATION");
          if (payload != nullptr) {
            ENTT_ID_TYPE id;
            assert(payload->DataSize == sizeof(id));
            memcpy(&id, payload->Data, sizeof(id));
            registry.emplace<Components::Animation>(*selected_entity, id, /*dummy=*/0u);
          }
          ImGui::EndDragDropTarget();
        }

        if (registry.has<Components::Transform>(*selected_entity)) {
          if (ImGui::TreeNode("Transform Component")) {
            auto& transform = registry.get<Components::Transform>(*selected_entity);
            ImGui::gizmo3D("##Dir1", transform.position, transform.orientation);

            ImGui::InputFloat3("Translation", glm::value_ptr(transform.position));
            auto euler_angles = glm::degrees(glm::eulerAngles(transform.orientation));
            ImGui::InputScalarN("Rotation",
                                ImGuiDataType_Float,
                                glm::value_ptr(euler_angles),
                                3,
                                nullptr,
                                nullptr,
                                "%.3f°");
            transform.orientation = glm::radians(euler_angles);
            ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale));
            ImGui::TreePop();
          }
        }

        if (registry.has<Components::Sky>(*selected_entity)) {
          if (ImGui::TreeNode("Sky Component")) {
            auto& sky = registry.get<Components::Sky>(*selected_entity);
            ImGui::gizmo3D("##direction_to_sun", sky.direction_to_sun);

            ImGui::InputFloat3("Direction To Sun", glm::value_ptr(sky.direction_to_sun));
            sky.direction_to_sun = glm::normalize(sky.direction_to_sun);
            ImGui::TreePop();
          }
        }
        if (registry.has<Components::Camera>(*selected_entity)) {
          if (ImGui::TreeNode("Camera Component")) {
            auto& camera = registry.get<Components::Camera>(*selected_entity);
            float fov = glm::degrees(camera.fov_y);
            ImGui::InputFloat("Vertical of view", &fov, 1.0f, 5.0f, "%.3f°");
            camera.fov_y = glm::radians(fov);
            ImGui::InputFloat("Near Plane", &camera.near);
            ImGui::InputFloat("Far Plane", &camera.far);
            ImGui::TreePop();
          }
        }
        if (registry.has<Components::Mesh>(*selected_entity)) {
          if (ImGui::TreeNode("Mesh Component")) {
            auto& mesh = registry.get<Components::Mesh>(*selected_entity);
            ImGui::Text("Name: %s", resource_manager.mesh_cache().handle(mesh.id)->name.c_str());
            ImGui::TreePop();
          }
        }
        if (registry.has<Components::Armature>(*selected_entity)) {
          if (ImGui::TreeNode("Armature Component")) {
            auto& armature = registry.get<Components::Armature>(*selected_entity);
            char matrix_name[256];
            uint32_t i = 0;
            for (auto& joint : armature.joints) {
              ImGui::Text("Joint %d", i);
              snprintf(matrix_name, sizeof(matrix_name), "##joint%d - 0", i);
              ImGui::InputFloat4(matrix_name,
                                 glm::value_ptr(glm::transpose(joint)[0]),
                                 "%.3f",
                                 ImGuiInputTextFlags_ReadOnly);
              snprintf(matrix_name, sizeof(matrix_name), "##joint%d - 1", i);
              ImGui::InputFloat4(matrix_name,
                                 glm::value_ptr(glm::transpose(joint)[1]),
                                 "%.3f",
                                 ImGuiInputTextFlags_ReadOnly);
              snprintf(matrix_name, sizeof(matrix_name), "##joint%d - 2", i);
              ImGui::InputFloat4(matrix_name,
                                 glm::value_ptr(glm::transpose(joint)[2]),
                                 "%.3f",
                                 ImGuiInputTextFlags_ReadOnly);
              snprintf(matrix_name, sizeof(matrix_name), "##joint%d - 3", i);
              ImGui::InputFloat4(matrix_name,
                                 glm::value_ptr(glm::transpose(joint)[3]),
                                 "%.3f",
                                 ImGuiInputTextFlags_ReadOnly);
              i++;
            }
            ImGui::TreePop();
          }
        }
        if (registry.has<Components::Animation>(*selected_entity)) {
          if (ImGui::TreeNode("Animation Component")) {
            auto& animation = registry.get<Components::Animation>(*selected_entity);
            ImGui::Text("Name: %s",
                        resource_manager.animation_cache().handle(animation.id)->name.c_str());
            ImGui::TreePop();
          }
        }
      }
      ImGui::End();
    }
  }

  // Rendering
  ImGui::Render();
}

void
Ui::draw() const
{
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Multi viewport handling
  SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
  SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();
  SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
}

bool
Ui::process_event(const Window& window, const SDL_Event& event)
{
  ImGui_ImplSDL2_ProcessEvent(&event);
  if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
      event.window.windowID == SDL_GetWindowID(window.get_native_handle())) {
    return false;
  }
  return true;
}

bool
Ui::has_mouse() const
{
  auto& io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

bool
Ui::mouse_over_scene_window() const
{
  return scene_window_hovered_;
}
