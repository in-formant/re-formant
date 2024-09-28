cmake_minimum_required(VERSION 3.13...3.22 FATAL_ERROR)

project(ReFormant LANGUAGES C CXX)
set(PROJECT_VERSION 0.1-alpha.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CMakeRC REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE was not set: defaults to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

    set(WINVER _WIN32_WINNT_WIN10)
    add_compile_definitions(WINVER=${WINVER} _WIN32_WINNT=${WINVER})
endif ()

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8)
endif ()

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tests)