set(PROJ_NAME bvh-loader)

set(SOURCES
  util.h
  bvh.cpp
  bvh.h
  bvhloader.cpp
  bvhloader.h
)

add_library(${PROJ_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJ_NAME} ${CMAKE_CURRENT_SOURCE_DIR})
