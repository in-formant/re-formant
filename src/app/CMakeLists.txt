add_executable(ReFormant
    audio/audiodevices.cpp
    audio/audiodevices.h
    audio/audioinput.cpp
    audio/audioinput.h
    audio/audiooutput.cpp
    audio/audiooutput.h
    audio/setup_audio.cpp
    audio/setup_audio.h
    audiofiles/audioread.cpp
    audiofiles/audiowrite.cpp
    audiofiles/fileformats.cpp
    audiofiles/audiofiles.h
    processing/audiotrack.cpp
    processing/audiotrack.h
    processing/consumerthread.cpp
    processing/consumerthread.h
    processing/denoiser.cpp
    processing/denoiser.h
    processing/pitchcontroller.cpp
    processing/pitchcontroller.h
    processing/processingthread.cpp
    processing/processingthread.h
    processing/resampler.cpp
    processing/resampler.h
    processing/spectrogramcontroller.cpp
    processing/spectrogramcontroller.h
    readerwriterqueue/atomicops.h
    readerwriterqueue/readerwriterqueue.h
    settings/settings_ini.cpp
    settings/settings_ini.h
    settings/settings_none.h
    settings/settings.cpp
    settings/settings.h
    ui/render.cpp
    ui/setup_glfw.cpp
    ui/setup_imgui.cpp
    ui/style.cpp
    ui/ui_audiosettings.cpp
    ui/ui_dockspace.cpp
    ui/ui_profiler.cpp
    ui/ui_spectrogram.cpp
    ui/ui_private.h
    ui/ui.h
    main.cpp
    memusage.cpp
    memusage.h
    state.h
    mini/ini.h
)

target_include_directories(ReFormant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ReFormant
    PRIVATE PortAudio
            glad glm::glm freetype
            ImGui::Core ImGui::FreeType
            ImPlot ImFileDialog
            fftw3f speex_resampler
            sndfile rnnoise
)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set_property(TARGET ReFormant
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(WIN32)
    target_sources(ReFormant PRIVATE reformant.manifest)
    target_compile_definitions(ReFormant PRIVATE "IMGUI_ENABLE_WIN32_DEFAULT_IME_FUNCTIONS")

    # Only disable console in Release build
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        if(MSVC)
            target_link_options(ReFormant PRIVATE /subsystem:windows)
        else()
            target_link_options(ReFormant PRIVATE -mwindows)
        endif()
        target_compile_definitions(ReFormant PRIVATE "WINMAIN")
    endif()

    target_link_libraries(ReFormant PRIVATE psapi)
endif()

if(MSVC)
    target_compile_options(ReFormant PRIVATE /utf-8)
endif()

if(EMSCRIPTEN)
    set(EM_GLFW_FLAG "SHELL:-s USE_GLFW=3")
    target_compile_options(ReFormant PRIVATE ${EM_GLFW_FLAG})
    target_link_options(ReFormant PRIVATE ${EM_GLFW_FLAG})
else()
    target_link_libraries(ReFormant PRIVATE glfw)
endif()
target_link_libraries(ReFormant PRIVATE ImGui::ImplGlfw)

find_package(OpenGL REQUIRED)
target_link_libraries(ReFormant PRIVATE OpenGL::GL ImGui::ImplOpenGL3)

if(USE_ASAN)
    target_compile_options(ReFormant PRIVATE -fsanitize=address)
    target_link_options(ReFormant PRIVATE -fsanitize=address)
endif()

if(USE_USAN)
    target_compile_options(ReFormant PRIVATE -fsanitize=undefined)
    target_link_options(ReFormant PRIVATE -fsanitize=undefined)
endif()