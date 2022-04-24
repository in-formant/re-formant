FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        ec48681455694c85982861dfa3f304fd544d3137 # latest docking
)

if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    add_library(ImGui_Core STATIC)
    target_include_directories(ImGui_Core PUBLIC ${imgui_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_sources(ImGui_Core PRIVATE
        ${imgui_SOURCE_DIR}/imconfig.h
        ${imgui_SOURCE_DIR}/imgui.h
        ${imgui_SOURCE_DIR}/imgui_internal.h)
    file(GLOB _imgui_sources ${imgui_SOURCE_DIR}/imgui*.cpp)
    foreach(SOURCE ${_imgui_sources})
        target_sources(ImGui_Core 
                PRIVATE ${SOURCE})
    endforeach()
    target_compile_definitions(ImGui_Core PUBLIC "IMGUI_USE_STB_SPRINTF" "ImDrawIdx=unsigned int")
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(ImGui_Core PUBLIC imm32 dwmapi)
    endif()
    if(APPLE)
        target_compile_definitions(ImGui_Core PUBLIC "IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS")
        target_link_libraries(ImGui_Core PUBLIC "-framework ApplicationServices")
    endif()
    add_library(ImGui::Core ALIAS ImGui_Core)

    function(imgui_library TARGET)
        cmake_parse_arguments(TARGET "" "" "HEADERS;SOURCES;DEFINITIONS;TARGETS;INCLUDE_DIRECTORIES" ${ARGN})
        set(_target ImGui_${TARGET})
        add_library(${_target} STATIC)
        target_link_libraries(${_target} PUBLIC ImGui_Core)
        foreach(SOURCE ${TARGET_SOURCES})
            target_sources(${_target}
                    PRIVATE ${imgui_SOURCE_DIR}/${SOURCE})
        endforeach()
        foreach(DEFINITION ${TARGET_DEFINITIONS})
            target_compile_definitions(${_target}
                PUBLIC ${DEFINITION})
        endforeach()
        target_include_directories(${_target}
                PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>)
        foreach(INCLUDE_DIRECTORY ${TARGET_INCLUDE_DIRECTORIES})
            target_include_directories(${_target}
                    PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/${INCLUDE_DIRECTORY}>)
        endforeach()
        foreach(DEFINITION ${TARGET_DEFINITIONS})
            target_compile_definitions(${_target}
                PUBLIC ${DEFINITION})
        endforeach()
        foreach(INTERFACE_TARGET ${TARGET_TARGETS})
            target_link_libraries(${_target}
                PUBLIC ${INTERFACE_TARGET})
        endforeach()
        add_library(ImGui::${TARGET} ALIAS ${_target})
    endfunction()

    imgui_library(ImplGlfw
        HEADERS backends/imgui_impl_glfw.h
        SOURCES backends/imgui_impl_glfw.cpp
        INCLUDE_DIRECTORIES backends)
        
    if(EMSCRIPTEN)
        set(EM_GLFW_FLAG "SHELL:-s USE_GLFW=3")
        target_compile_options(ImGui_ImplGlfw PRIVATE ${EM_GLFW_FLAG})
        target_link_options(ImGui_ImplGlfw PRIVATE ${EM_GLFW_FLAG})
    else()
        target_link_libraries(ImGui_ImplGlfw PRIVATE glfw)
    endif()

    imgui_library(ImplOpenGL2
        HEADERS backends/imgui_impl_opengl2.h
        SOURCES backends/imgui_impl_opengl2.cpp
        INCLUDE_DIRECTORIES backends)

    imgui_library(ImplOpenGL3
        HEADERS
            backends/imgui_impl_opengl3.h
            backends/imgui_impl_opengl3_loader.h
        SOURCES backends/imgui_impl_opengl3.cpp
        DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GLAD=1"
        INCLUDE_DIRECTORIES backends)

    imgui_library(FreeType
        HEADERS misc/freetype/imgui_freetype.h
        SOURCES misc/freetype/imgui_freetype.cpp
        DEFINITIONS
            "IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION"
            "IMGUI_ENABLE_FREETYPE"
        TARGETS freetype)

    imgui_library(StdLib
        HEADERS misc/cpp/imgui_stdlib.h
        SOURCES misc/cpp/imgui_stdlib.cpp)
endif()