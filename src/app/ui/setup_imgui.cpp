#include <glad/glad.h>
// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <ImFileDialog.h>
#include <misc/freetype/imgui_freetype.h>
// clang-format on

#include "../state.h"
#include "font_faRegular.h"
#include "font_faSolid.h"
#include "font_interMedium.h"
#include "ui.h"

void reformant::ui::setupImGui(AppState& appState) {
    // Set up ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    reformant::ui::setStyle(true, 1.0, appState.ui.scalingFactor);

    ImGui_ImplGlfw_InitForOpenGL(appState.ui.window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    glEnable(GL_MULTISAMPLE);

    // Set up fonts.

    const float fontSize = 16.0f * appState.ui.scalingFactor;
    const float iconVerticalOffset = 1.0f * appState.ui.scalingFactor;

    io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting;

    io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        g_interMedium_compressed_data_base85, fontSize, nullptr,
        io.Fonts->GetGlyphRangesDefault());

    static const ImWchar iconsRange[] = {0xE000, 0xF8FF, 0};
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = fontSize;
    config.GlyphOffset.y = iconVerticalOffset;
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        g_faRegular_compressed_data_base85, fontSize, &config, iconsRange);

    config.MergeMode = false;
    config.GlyphOffset.y = 0;
    appState.ui.faSolid = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        g_faSolid_compressed_data_base85, fontSize, &config, iconsRange);

    io.Fonts->Build();

    // Set up ImFileDialog

    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h,
                                                   char fmt) -> void* {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     (fmt == 0) ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        return (void*)(uintptr_t)tex;
    };

    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        GLuint texID = (GLuint)(uintptr_t)tex;
        glDeleteTextures(1, &texID);
    };
}
