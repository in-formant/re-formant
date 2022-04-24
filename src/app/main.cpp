#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <portaudio.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "audio/setup_audio.h"
#include "processing/consumerthread.h"
#include "processing/pitchcontroller.h"
#include "processing/processingthread.h"
#include "processing/spectrogramcontroller.h"
#include "state.h"
#include "ui/ui.h"

namespace {
reformant::Settings instantiateSettings();
}  // namespace

int main(int argc, char* argv[]) {
    reformant::AppState appState;
    appState.settings = instantiateSettings();

    reformant::ui::setupGlfw(appState);

    PaError paErr = Pa_Initialize();
    if (paErr != paNoError) {
        std::cerr << "Failed to initialize PortAudio: "
                  << Pa_GetErrorText(paErr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    reformant::ui::setupImGui(appState);

    reformant::setupAudio(appState);

    reformant::SpectrogramController spectrogramController(appState);
    spectrogramController.setFftLength(appState.settings.fftLength());
    appState.spectrogramController = &spectrogramController;

    reformant::PitchController pitchController(appState);
    appState.pitchController = &pitchController;

    appState.ui.showAudioSettings = appState.settings.showAudioSettings();

    appState.settings.spectrumPlotRatios(appState.ui.spectrumPlotRatios);
    appState.ui.plotTimeMin = 0;
    appState.ui.plotTimeMax = 10;
    appState.ui.plotFreqMin = appState.settings.spectrumFreqMin();
    appState.ui.plotFreqMax = appState.settings.spectrumFreqMax();
    appState.ui.plotFreqScale = appState.settings.spectrumFreqScale();
    appState.ui.spectrumMinDb = appState.settings.spectrumMinDb();
    appState.ui.spectrumMaxDb = appState.settings.spectrumMaxDb();

    appState.ui.isRecording = false;
    appState.ui.isInTimeScrollAnimation = false;

    appState.ui.averageProcessingTime = -1;

    ImGuiIO& io = ImGui::GetIO();

    // Start the audio consumer thread.
    reformant::ConsumerThread consumerThread(appState);
    consumerThread.start();

    // Start the audio processing thread.
    reformant::ProcessingThread processingThread(appState);
    processingThread.start();
    appState.processingThread = &processingThread;

    // Clear color
    ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(appState.ui.window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
        // tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data
        // to your main application, or clear/overwrite your copy of the mouse
        // data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
        // data to your main application, or clear/overwrite your copy of the
        // keyboard data. Generally you may always pass all inputs to dear
        // imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        reformant::ui::render(appState);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(appState.ui.window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                     clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we
        // save/restore it to make it easier to paste this code elsewhere.
        // For this specific demo app we could also call
        // glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(appState.ui.window);
    }

    // Cleanup
    consumerThread.terminate();
    processingThread.terminate();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    appState.audioInput.closeStream();
    Pa_Terminate();

    glfwDestroyWindow(appState.ui.window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

#define SETTINGS_NOOP 0
#define SETTINGS_INI 1

#if defined(_WIN32) || defined(_WIN64)  // Windows
    #define SETTINGS SETTINGS_INI
#elif defined(__CYGWIN__) && !defined(_WIN32)  // Windows (Cygwin)
    #define SETTINGS SETTINGS_INI
#elif defined(__ANDROID__)  // Android (implies Linux)
    #define SETTINGS SETTINGS_NOOP
#elif defined(__linux__)  // Linux
    #define SETTINGS SETTINGS_INI
#elif defined(__APPLE__) && defined(__MACH__)  // Apple OSX and iOS (Darwin)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1  // Apple iOS
        #define SETTINGS SETTINGS_NOOP
    #elif TARGET_OS_IPHONE == 1  // Apple iOS
        #define SETTINGS SETTINGS_NOOP
    #elif TARGET_OS_MAC == 1  // Apple OSX
        #define SETTINGS SETTINGS_INI
    #endif
#else
    #define SETTINGS SETTINGS_NOOP
#endif

#if SETTINGS == SETTINGS_NOOP
    #warning "Target system not supported yet. Using no-op settings backend."
    #define SystemSettingsBackend() reformant::SettingsBackend()
#elif SETTINGS == SETTINGS_INI
    #include "settings/settings_ini.h"
    #define SystemSettingsBackend() reformant::IniSettingsBackend()
#endif

namespace {
reformant::Settings instantiateSettings() {
    return reformant::Settings(SystemSettingsBackend());
}
}  // namespace

#if defined(_WIN32) && defined(WINMAIN)

    #include <windows.h>

static inline char* wideToMulti(int codePage, const wchar_t* aw) {
    const int required =
        WideCharToMultiByte(codePage, 0, aw, -1, NULL, 0, NULL, NULL);
    char* result = new char[required];
    WideCharToMultiByte(codePage, 0, aw, -1, result, required, NULL, NULL);
    return result;
}

extern "C" int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int argc;
    wchar_t** argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) return -1;
    char** argv = new char*[argc + 1];
    for (int i = 0; i < argc; ++i) argv[i] = wideToMulti(CP_ACP, argvW[i]);
    argv[argc] = nullptr;
    LocalFree(argvW);
    const int exitCode = main(argc, argv);
    for (int i = 0; i < argc && argv[i]; ++i) delete[] argv[i];
    delete[] argv;
    return exitCode;
}

#endif