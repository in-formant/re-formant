project(glad)

add_library(glad INTERFACE)

target_sources(glad INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c>)
target_include_directories(glad INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)