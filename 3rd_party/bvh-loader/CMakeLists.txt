set(PROJ_NAME bvh-loader)

set(SOURCES
  util.h
  bvh.cpp
  bvh.h
  bvhloader.cpp
  bvhloader.h
)

add_library(${PROJ_NAME} STATIC ${SOURCES})
target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
