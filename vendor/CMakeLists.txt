include(FetchContent)

# -- FetchContent_Declare everything

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        45ce5ddd197d5c58f50fdd3296a5131c894e5527 # 3.3-stable
)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e # latest
)

# FreeType
FetchContent_Declare(
    freetype
    URL      https://download.savannah.gnu.org/releases/freetype/freetype-2.11.1.tar.gz
    URL_HASH SHA256=F8DB94D307E9C54961B39A1CC799A67D46681480696ED72ECF78D4473770F09B
)

# portaudio
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG        af42647e3804f6a2472a00d943aaeedd641f7966
    #URL      http://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz
    #URL_HASH SHA256=47EFBF42C77C19A05D22E627D42873E991EC0C1357219C0D74CE6A2948CB2DEF
)

# fftw3
FetchContent_Declare(
    fftw3
    URL      https://fftw.org/fftw-3.3.10.tar.gz
    URL_HASH SHA256=56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467
)

# libsndfile
FetchContent_Declare(
    libsndfile
    URL      https://github.com/libsndfile/libsndfile/releases/download/1.1.0/libsndfile-1.1.0.tar.xz
    URL_HASH SHA256=0f98e101c0f7c850a71225fb5feaf33b106227b3d331333ddc9bacee190bcf41
)

# -- GLFW setttings 

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")

# -- freetype settings

foreach(_freetype_option "SKIP_INSTALL_HEADERS" "SKIP_INSTALL_LIBRARIES"
            "CMAKE_DISABLE_FIND_PACKAGE_PNG" "CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz"
            "CMAKE_DISABLE_FIND_PACKAGE_BZip2" "CMAKE_DISABLE_FIND_PACKAGE_ZLIB"
            "CMAKE_DISABLE_FIND_PACKAGE_BROTLI")
    set(${_freetype_option} TRUE CACHE INTERNAL "")
endforeach()

# -- portaudio settings

set(PA_BUILD_STATIC ON CACHE INTERNAL "")
set(PA_BUILD_SHARED OFF CACHE INTERNAL "")
set(PA_ENABLE_DEBUG_OUTPUT OFF CACHE INTERNAL "")

# -- fftw3 settings

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_TESTS OFF CACHE INTERNAL "")
set(ENABLE_SSE ON CACHE INTERNAL "")
set(ENABLE_FLOAT ON CACHE INTERNAL "")

# -- libsndfile settings

set(BUILD_TESTING OFF CACHE INTERNAL "")
set(BUILD_PROGRAMS OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_REGTEST OFF CACHE INTERNAL "")
set(ENABLE_CPACK OFF CACHE INTERNAL "")
set(ENABLE_PACKAGE_CONFIG OFF CACHE INTERNAL "")
set(INSTALL_PKGCONFIG_MODULE OFF CACHE INTERNAL "")

# -- make available

FetchContent_GetProperties(freetype)
if(NOT freetype_POPULATED)
    FetchContent_Populate(freetype)

    file(GLOB _freetype_patch "${CMAKE_CURRENT_SOURCE_DIR}/freetype/*")
    file(COPY ${_freetype_patch} DESTINATION ${freetype_SOURCE_DIR})

    add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR})
endif()

FetchContent_GetProperties(fftw3)
if(NOT fftw3_POPULATED)
    FetchContent_Populate(fftw3)

    file(GLOB _fftw3_patch "${CMAKE_CURRENT_SOURCE_DIR}/fftw3/*")
    file(COPY ${_fftw3_patch} DESTINATION ${fftw3_SOURCE_DIR})

    add_subdirectory(${fftw3_SOURCE_DIR} ${fftw3_BINARY_DIR})

    target_include_directories(fftw3f INTERFACE $<BUILD_INTERFACE:${fftw3_SOURCE_DIR}/api>)
    if(WIN32)
        target_compile_definitions(fftw3f PRIVATE HAVE_ALIGNED_MALLOC)
    endif()
endif()

FetchContent_MakeAvailable(glfw glm portaudio libsndfile)

# -- imgui

add_subdirectory(imgui)
add_subdirectory(implot)
add_subdirectory(imfiledialog)

add_subdirectory(glad)

add_subdirectory(speexdsp)