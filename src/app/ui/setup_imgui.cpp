#include <glad/glad.h>
#include <cmrc/cmrc.hpp>
// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <ImFileDialog.h>
#include <misc/freetype/imgui_freetype.h>
// clang-format on

#include "../state.h"
#include "ui.h"

CMRC_DECLARE(fonts);

void reformant::ui::setupImGui(AppState& appState) {
    // Set up ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    reformant::ui::setStyle(true, 1.0, appState.ui.scalingFactor);

    ImGui_ImplGlfw_InitForOpenGL(appState.ui.window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    glEnable(GL_MULTISAMPLE);

    // Set up fonts

    const float fontSize = 17.0f * appState.ui.scalingFactor;
    const float iconVerticalOffset = 1.0f * appState.ui.scalingFactor;

    io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.Fonts->FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting;

    // Font files.
    auto fs = cmrc::fonts::get_filesystem();

    auto faRegular = fs.open("faRegular.otf");
    auto faSolid = fs.open("faSolid.otf");
    auto interMedium = fs.open("interMedium.ttf");

    auto faRegularBuf = std::make_unique<char[]>(faRegular.size());
    auto faSolidBuf = std::make_unique<char[]>(faSolid.size());
    auto interMediumBuf = std::make_unique<char[]>(interMedium.size());

    std::copy(faRegular.begin(), faRegular.end(), faRegularBuf.get());
    std::copy(faSolid.begin(), faSolid.end(), faSolidBuf.get());
    std::copy(interMedium.begin(), interMedium.end(), interMediumBuf.get());

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;

    static constexpr ImWchar interRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0370, 0x03FF, // Greek and Coptic
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };

    static constexpr ImWchar iconsRange[] = {
        0xE000, 0xF8FF,
        0,
    };

    io.Fonts->AddFontFromMemoryTTF(interMediumBuf.get(), interMedium.size(),
                                   fontSize,
                                   &config,
                                   interRanges);

    config.MergeMode = true;

    config.GlyphMinAdvanceX = fontSize;
    config.GlyphOffset.y = iconVerticalOffset;
    io.Fonts->AddFontFromMemoryTTF(faRegularBuf.get(), faRegular.size(), fontSize,
                                   &config,
                                   iconsRange);

    config.MergeMode = false;
    config.GlyphOffset.y = 0;
    appState.ui.faSolid = io.Fonts->AddFontFromMemoryTTF(
        faSolidBuf.get(), faSolid.size(), fontSize, &config, iconsRange);

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
