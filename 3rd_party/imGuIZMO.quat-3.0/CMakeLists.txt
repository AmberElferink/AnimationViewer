file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/imGuIZMO.quat/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/imGuIZMO.quat/*.h")

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(imGuIZMOquat STATIC ${SOURCES} ${HEADERS})
target_link_libraries(imGuIZMOquat PUBLIC imgui glm)
target_compile_definitions(imGuIZMOquat PUBLIC VGIZMO_USES_GLM IMGUIZMO_IMGUI_FOLDER=${CMAKE_SOURCE_DIR}/3rd_party/imgui-docking/)
target_include_directories(imGuIZMOquat
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imGuIZMO.quat>
)
