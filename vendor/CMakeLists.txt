include(FetchContent)

find_package(glfw3 REQUIRED)

find_package(glm REQUIRED)

find_package(Freetype REQUIRED)

find_package(FFTW REQUIRED COMPONENTS FLOAT_LIB)

find_package(sndfile REQUIRED)

find_package(Portaudio REQUIRED)

add_subdirectory(imgui)
add_subdirectory(implot)
add_subdirectory(imfiledialog)

add_subdirectory(glad)

add_subdirectory(speexdsp)

set(BUILD_EXAMPLES OFF)
add_subdirectory(cppflow)