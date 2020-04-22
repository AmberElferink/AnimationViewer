file(GLOB imgui_sources *.cpp examples/imgui_impl_sdl.cpp examples/imgui_impl_opengl3.cpp)
file(GLOB imgui_headers *.h examples/imgui_impl_sdl.h examples/imgui_impl_opengl3.h)
add_library(imgui STATIC ${imgui_sources} ${imgui_headers})
if (EMSCRIPTEN)
  set_target_properties(imgui PROPERTIES
    COMPILE_FLAGS "-s USE_SDL=2 -s USE_PTHREADS=1"
    LINK_FLAGS "-s USE_SDL=2 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s USE_WEBGL2=1 -s USE_PTHREADS=1")
else()
  target_link_libraries(imgui PRIVATE SDL2::SDL2 glad)
endif()
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/examples)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
