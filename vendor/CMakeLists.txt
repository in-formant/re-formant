include(FetchContent)

# -- FetchContent_Declare everything

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 45ce5ddd197d5c58f50fdd3296a5131c894e5527 # 3.3-stable
)

# glm
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # latest
)

# FreeType
FetchContent_Declare(
        freetype
        URL https://download.savannah.gnu.org/releases/freetype/freetype-2.11.1.tar.gz
        URL_HASH SHA256=F8DB94D307E9C54961B39A1CC799A67D46681480696ED72ECF78D4473770F09B
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# portaudio
FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG af42647e3804f6a2472a00d943aaeedd641f7966
)

# fftw3
FetchContent_Declare(
        fftw3
        URL https://fftw.org/fftw-3.3.10.tar.gz
        URL_HASH SHA256=56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# libsndfile
FetchContent_Declare(
        libsndfile
        GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
        GIT_TAG a17e32fda6ed6883bebe0d5f7e1c83cd88409bd6 # latest (because Lame dependency search is broken on 1.1.0)
)

# rnnoise
FetchContent_Declare(
        rnnoise
        GIT_REPOSITORY https://github.com/mumble-voip/rnnoise.git # mumble's fork has the msvc vla fix
        GIT_TAG a9afcccb3c4f01e6e76a1ef67ccb58b7799856bf # latest
)

# -- GLFW setttings 

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")

# -- freetype settings

foreach (_freetype_option "SKIP_INSTALL_HEADERS" "SKIP_INSTALL_LIBRARIES"
        "CMAKE_DISABLE_FIND_PACKAGE_PNG" "CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz"
        "CMAKE_DISABLE_FIND_PACKAGE_BZip2" "CMAKE_DISABLE_FIND_PACKAGE_ZLIB"
        "CMAKE_DISABLE_FIND_PACKAGE_BROTLI")
    set(${_freetype_option} TRUE CACHE INTERNAL "")
endforeach ()

# -- portaudio settings

set(PA_BUILD_STATIC ON CACHE INTERNAL "")
set(PA_BUILD_SHARED OFF CACHE INTERNAL "")
set(PA_ENABLE_DEBUG_OUTPUT OFF CACHE INTERNAL "")

# -- fftw3 settings

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_TESTS OFF CACHE INTERNAL "")
set(ENABLE_SSE ON CACHE INTERNAL "")
set(ENABLE_FLOAT ON CACHE INTERNAL "")

if (CMAKE_CXX_COMPILER_ID STREQUAL MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL MSVC)
    set(LIBM_LIBRARY OFF CACHE INTERNAL "")
endif ()

# -- libsndfile settings

set(BUILD_TESTING OFF CACHE INTERNAL "")
set(BUILD_PROGRAMS OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_REGTEST OFF CACHE INTERNAL "")
set(ENABLE_CPACK OFF CACHE INTERNAL "")
set(ENABLE_PACKAGE_CONFIG OFF CACHE INTERNAL "")
set(INSTALL_PKGCONFIG_MODULE OFF CACHE INTERNAL "")

# -- make available

FetchContent_MakeAvailable(freetype)
#FetchContent_GetProperties(freetype)
#if(NOT freetype_POPULATED)
#    FetchContent_Populate(freetype)

file(GLOB _freetype_patch "${CMAKE_CURRENT_SOURCE_DIR}/freetype/*")
file(COPY ${_freetype_patch} DESTINATION ${freetype_SOURCE_DIR})

#    add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR})
#endif()

FetchContent_MakeAvailable(fftw3)
#FetchContent_GetProperties(fftw3)
#if(NOT fftw3_POPULATED)
#    FetchContent_Populate(fftw3)

file(GLOB _fftw3_patch "${CMAKE_CURRENT_SOURCE_DIR}/fftw3/*")
file(COPY ${_fftw3_patch} DESTINATION ${fftw3_SOURCE_DIR})

#    add_subdirectory(${fftw3_SOURCE_DIR} ${fftw3_BINARY_DIR})

target_include_directories(fftw3f INTERFACE $<BUILD_INTERFACE:${fftw3_SOURCE_DIR}/api>)
if (WIN32)
    target_compile_definitions(fftw3f PRIVATE HAVE_ALIGNED_MALLOC)
endif ()
#endif()

FetchContent_MakeAvailable(glfw glm portaudio)

FetchContent_MakeAvailable(libsndfile)
#FetchContent_GetProperties(libsndfile)
#if(NOT libsndfile_POPULATED)
#    FetchContent_Populate(libsndfile)

file(GLOB _libsndfile_patch "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile/*")
file(COPY ${_libsndfile_patch} DESTINATION ${libsndfile_SOURCE_DIR})

#    add_subdirectory(${libsndfile_SOURCE_DIR} ${libsndfile_BINARY_DIR})
#endif()

FetchContent_MakeAvailable(rnnoise)
#FetchContent_GetProperties(rnnoise)
#if(NOT rnnoise_POPULATED)
#    FetchContent_Populate(rnnoise)

file(GLOB _rnnoise_patch "${CMAKE_CURRENT_SOURCE_DIR}/rnnoise/*")
file(COPY ${_rnnoise_patch} DESTINATION ${rnnoise_SOURCE_DIR})

#    add_subdirectory(${rnnoise_SOURCE_DIR} ${rnnoise_BINARY_DIR})
#endif()

# -- imgui

add_subdirectory(imgui)
add_subdirectory(implot)
add_subdirectory(imfiledialog)

add_subdirectory(glad)

add_subdirectory(speexdsp)
