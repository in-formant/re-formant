#include <ImFileDialog.h>

#include <iostream>

#include "../audiofiles/audiofiles.h"
#include "ui_private.h"

void reformant::ui::dockspace(AppState& appState) {
    ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("Dockspace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceId = ImGui::GetID("Dockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f),
                         ImGuiDockNodeFlags_PassthruCentralNode);
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "CTRL+O", nullptr)) {
                ifd::FileDialog::Instance().Open(
                    "AudioFileOpenDialog", "Open an audio file",
                    reformant::getAudioFileReadFilter());
            }

            if (ImGui::MenuItem("Save", "CTRL+S", nullptr)) {
                ifd::FileDialog::Instance().Save(
                    "AudioFileSaveDialog", "Save an audio file",
                    reformant::getSupportedAudioFormats().filter);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Audio settings", nullptr,
                            &appState.ui.showAudioSettings);
            ImGui::MenuItem("Profiler", nullptr, &appState.ui.showProfiler);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();

    if (ifd::FileDialog::Instance().IsDone("AudioFileOpenDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const auto filePath =
                ifd::FileDialog::Instance().GetResult().string();

            std::vector<float> data;
            int sampleRate;
            if (reformant::readAudioFile(filePath, data, &sampleRate)) {
                std::lock_guard lock(appState.audioTrack.mutex());
                appState.audioTrack.append(data, sampleRate);
            }
        }
        std::cout << "audio file read filter: "
                  << reformant::getAudioFileReadFilter() << std::endl;
        ifd::FileDialog::Instance().Close();
    }

    if (ifd::FileDialog::Instance().IsDone("AudioFileSaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const auto filePath =
                ifd::FileDialog::Instance().GetResult().string();
            const size_t formatIndex =
                ifd::FileDialog::Instance().GetSelectedFilter();

            const int format =
                reformant::getSupportedAudioFormats().formats[formatIndex];

            appState.audioTrack.mutex().lock();
            const auto data = appState.audioTrack.data();
            const int sampleRate = appState.audioTrack.sampleRate();
            appState.audioTrack.mutex().unlock();

            if (reformant::writeAudioFile(filePath, format, data, sampleRate)) {
            }
        }
        std::cout << "audio file write filter: "
                  << reformant::getSupportedAudioFormats().filter << std::endl;
        ifd::FileDialog::Instance().Close();
    }
}