file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*")

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,readability-*)
add_library(OpenFBX STATIC ${SOURCES})
target_include_directories(OpenFBX
  PUBLIC
  $<INSTALL_INTERFACE:src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

set_property(TARGET OpenFBX PROPERTY CXX_STANDARD 17)
