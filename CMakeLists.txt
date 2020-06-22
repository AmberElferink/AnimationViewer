cmake_minimum_required(VERSION 3.15)
project(AnimationViewer)

find_program(GLSLVALIDATOR glslangValidator)

option(ANIMATIONVIEWER_ENABLE_BLENDING "Enable experimental joint blending" OFF)

file(GLOB sources CONFIGURE_DEPENDS src/*.cpp src/materials/*.cpp src/hittable/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/*.h)
file(GLOB_RECURSE private_impl_sources CONFIGURE_DEPENDS src/private_impl/*)

file(GLOB shaders CONFIGURE_DEPENDS src/private_impl/graphics/shaders/*.glsl)
file(GLOB shader_headers CONFIGURE_DEPENDS src/private_impl/graphics/shaders/*.h)

set(assets
  )

# Tell CMake how to compile shaders into SPIV, then into headers
foreach(shader ${shaders})
  file(RELATIVE_PATH shader_relative_path "${CMAKE_CURRENT_SOURCE_DIR}" "${shader}")
  get_filename_component(shader_relative_directory ${shader_relative_path} DIRECTORY)
  get_filename_component(shader_name ${shader_relative_path} NAME)
  string(REGEX REPLACE "\\." "_" shader_name_underscored ${shader_name})
  set(compiled_shader ${CMAKE_CURRENT_BINARY_DIR}/${shader_relative_directory}/${shader_name_underscored}.h)
  add_custom_command(
    OUTPUT ${compiled_shader}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${shader_relative_directory}
    COMMAND
    ${GLSLVALIDATOR}
    --target-env opengl
    -o ${compiled_shader}
    --vn ${shader_name_underscored}
    "$<$<CONFIG:debug>:-g -Od>$<$<CONFIG:relwithdebinfo>:-g>$<$<CONFIG:minsizerel>:-Os>"
    ${shader}
    DEPENDS ${shader} ${shader_headers}
  )
  list(APPEND sources ${compiled_shader})
endforeach()

# Tell CMake how to include assets
foreach(asset ${assets})
  get_filename_component(asset_name ${asset} NAME)
  set(asset_output ${CMAKE_CURRENT_BINARY_DIR}/${asset_name})
  add_custom_command(
    OUTPUT ${asset_output}
    COMMAND ${CMAKE_COMMAND} -E copy ${asset} ${asset_output}
    DEPENDS ${asset}
  )
  list(APPEND sources ${asset_output})
  set(EMBED_FILES "${EMBED_FILES} --embed-file ${asset_name}")
endforeach()

add_library(AnimationViewerLib STATIC ${sources} ${headers} ${private_impl_sources})
add_executable(AnimationViewer app/main.cpp)

if(EMSCRIPTEN)
  set_target_properties(AnimationViewerLib PROPERTIES
    COMPILE_FLAGS_DEBUG "-g4"
    LINK_FLAGS_DEBUG "-g4"
    COMPILE_FLAGS "-s USE_SDL=2 -s USE_PTHREADS=1"
    LINK_FLAGS "-s USE_SDL=2 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s USE_WEBGL2=1 -s USE_PTHREADS=1 -s TOTAL_MEMORY=128MB ${EMBED_FILES} --emrun"
    SUFFIX ".html"
    )
  set_target_properties(AnimationViewer PROPERTIES
    COMPILE_FLAGS_DEBUG "-g4"
    LINK_FLAGS_DEBUG "-g4"
    COMPILE_FLAGS "-s USE_SDL=2 -s USE_PTHREADS=1"
    LINK_FLAGS "-s USE_SDL=2 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s USE_WEBGL2=1 -s USE_PTHREADS=1 -s TOTAL_MEMORY=128MB ${EMBED_FILES} --emrun"
    SUFFIX ".html"
    )
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/backport")
  include(FetchContent)

  fetchcontent_declare(
    glm
    URL https://github.com/g-truc/glm/archive/0.9.9.8.tar.gz
    URL_HASH MD5=0e83d4a33c7d4044a2c35c8d6aba4671
  )
  fetchcontent_makeavailable(glm)

  fetchcontent_declare(
    entt
    URL https://github.com/skypjack/entt/archive/v3.3.2.tar.gz
    URL_HASH SHA256=150cd89b45bffbcd7643d39bbce282e8fa38307bb5ed25567b79e337376ba1c7
  )
  fetchcontent_makeavailable(entt)

  fetchcontent_declare(
    spirv-cross
    URL https://github.com/KhronosGroup/SPIRV-Cross/archive/2019-11-01.tar.gz
    URL_HASH MD5=841d5055ef364a4dea7b06378336212a
    CMAKE_CACHE_ARGS "-DSPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS:STRING=ON -DSPIRV_CROSS_CLI:STRING=OFF -DSPIRV_CROSS_ENABLE_TESTS:STRING=OFF"
  )
  fetchcontent_makeavailable(spirv-cross)
  set_target_properties(spirv-cross-glsl PROPERTIES
    COMPILE_FLAGS_DEBUG "-g4"
    LINK_FLAGS_DEBUG "-g4"
    COMPILE_FLAGS "-s USE_PTHREADS=1"
    LINK_FLAGS "-s USE_PTHREADS=1")
  target_compile_definitions(spirv-cross-glsl INTERFACE SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
  set(LIBRARIES glm EnTT glad imgui spirv-cross-glsl)
else()
  find_package(glm REQUIRED)
  find_package(EnTT REQUIRED)
  find_package(SDL2 REQUIRED)
  find_package(spirv_cross_core REQUIRED)
  find_package(spirv_cross_glsl REQUIRED)
  find_package(assimp CONFIG REQUIRED)
  if (UNIX)
    set(ASSIMP_LIB assimp)
  else()
    set(ASSIMP_LIB assimp::assimp)
  endif()
  set(LIBRARIES SDL2::SDL2 glad imgui spirv-cross-core spirv-cross-glsl glm EnTT::EnTT ${ASSIMP_LIB})
endif()

# 3d connexion 3d mouse
find_library(SPNAV_LIBRARY spnav)
if(SPNAV_LIBRARY)
  target_compile_definitions(AnimationViewerLib PRIVATE USE_SPNAV=1)
  target_link_libraries(AnimationViewerLib PRIVATE ${SPNAV_LIBRARY})
endif()

target_compile_definitions(AnimationViewerLib PRIVATE ANIMATIONVIEWER_ENABLE_BLENDING=$<BOOL:${ANIMATIONVIEWER_ENABLE_BLENDING}>)

add_subdirectory(3rd_party/anm)
add_subdirectory(3rd_party/glad)
add_subdirectory(3rd_party/imgui-docking)
add_subdirectory(3rd_party/imGuIZMO.quat-3.0)
add_subdirectory(3rd_party/l3d)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(3rd_party/ezc3d-Release_1.3.2)
add_subdirectory(3rd_party/OpenFBX)
add_subdirectory(3rd_party/bvh-loader)

target_include_directories(AnimationViewerLib
  PUBLIC
  include/
  PRIVATE
  3rd_party/stb
  3rd_party/nlohmann-json
  3rd_party/tinygltf
  # Compiled shaders
  ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(AnimationViewerLib PRIVATE ${LIBRARIES} l3d anm imGuIZMOquat ezc3d OpenFBX bvh-loader)
target_compile_definitions(AnimationViewerLib PRIVATE SDL_MAIN_HANDLED SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
if(MSVC)
  target_compile_options(AnimationViewerLib
    PUBLIC
    /arch:AVX2
    PRIVATE
    /W4
    /wd4505 # disable unused function
    /wd4324 # structure padded due to alignment specifer
    #/WX  # Warning are errors
    )
elseif(EMSCRIPTEN)
  target_compile_options(AnimationViewerLib
    PUBLIC
    -msimd128
    PRIVATE
    -fno-exceptions
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-format-nonliteral
    -Wno-dollar-in-identifier-extension
    -Wno-gnu-zero-variadic-macro-arguments
    #-Werror
    -pedantic
    -pedantic-errors
    )
else()
  target_compile_options(AnimationViewerLib
    PUBLIC
    -march=haswell
    PRIVATE
    -fno-exceptions
    -Wall
    -Wextra
    -Wno-unused-function
    -Wno-format-nonliteral
    -Werror
    -pedantic
    -pedantic-errors

    -Wcomment
    -Wimport
    -Winline
    -Wparentheses
    -Wreturn-type
    -Wstack-protector
    -Wstrict-aliasing
    -Wstrict-aliasing=2
    -Wswitch
    # -Wswitch-default
    -Wswitch-enum
    -Wuninitialized
    -Wmissing-braces
    -Wmissing-field-initializers
    -Wmissing-format-attribute
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wpointer-arith
    -Wredundant-decls
    -Wsequence-point
    -Wshadow
    -Wsign-compare
    -Wtrigraphs
    -Wunknown-pragmas
    -Wunreachable-code
    -Wunused-label
    # -Wunused-parameter
    -Wno-unused-parameter
    -Wno-unused-private-field
    -Wunused-value
    -Wunused-variable
    -Wvariadic-macros
    -Wvolatile-register-var
    -Wwrite-strings
    -Wdisabled-optimization
    -Winit-self
    -Winvalid-pch
    #-Wlong-long
    #-Weffc++
    #-Waggregate-return
    -Wchar-subscripts

    # -Wcast-align
    # -Wconversion
    # -Wpacked
    # -Wpadded
    # -Wcast-qual
    # -Wformat
    # -Wformat=2
    # -Wformat-security
    # -Wformat-y2k
    )
endif()
set_property(TARGET AnimationViewerLib PROPERTY CXX_STANDARD 20)

target_link_libraries(AnimationViewer PRIVATE AnimationViewerLib)
set_property(TARGET AnimationViewer PROPERTY CXX_STANDARD 20)
