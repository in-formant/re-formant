#include <ImFileDialog.h>

#include <iostream>

#include "../audiofiles/audiofiles.h"
#include "../processing/controller/pitchcontroller.h"
#include "../processing/controller/spectrogramcontroller.h"
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

    appState.audioTrack.mutex().lock();
    const int trackSampleRate = appState.audioTrack.sampleRate();
    appState.audioTrack.mutex().unlock();

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "CTRL+N", nullptr)) {
                std::lock_guard trackGuard(appState.audioTrack.mutex());
                if (appState.audioOutput.isPlaying()) appState.audioOutput.stopPlaying();
                appState.audioTrack.reset();
                appState.spectrogramController->setTime(0);
                appState.spectrogramController->forceClear();
                appState.pitchController->forceClear();
            }

            if (ImGui::MenuItem("Open", "CTRL+O", nullptr)) {
                ifd::FileDialog::Instance().Open("AudioFileOpenDialog",
                                                 "Open an audio file",
                                                 audiofiles::getReadFilter());
            }

            if (ImGui::MenuItem("Save", "CTRL+S", nullptr)) {
                ifd::FileDialog::Instance().Save(
                    "AudioFileSaveDialog", "Save an audio file",
                    audiofiles::getWriteFilter(trackSampleRate));
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Audio settings", nullptr, &appState.ui.showAudioSettings);
            ImGui::MenuItem("Display settings", nullptr,
                            &appState.ui.showDisplaySettings);
            ImGui::MenuItem("Profiler", nullptr, &appState.ui.showProfiler);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();

    if (ifd::FileDialog::Instance().IsDone("AudioFileOpenDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const auto filePath = ifd::FileDialog::Instance().GetResult().string();

            std::vector<float> data;
            int sampleRate;
            if (audiofiles::readFile(filePath, data, &sampleRate)) {
                std::lock_guard lock(appState.audioTrack.mutex());
                appState.audioTrack.append(data, sampleRate);
            }
        }
        ifd::FileDialog::Instance().Close();

        std::cout << "audio file read filter: " << audiofiles::getReadFilter()
                  << std::endl;
    }

    if (ifd::FileDialog::Instance().IsDone("AudioFileSaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const auto filePath = ifd::FileDialog::Instance().GetResult().string();
            const size_t formatIndex = ifd::FileDialog::Instance().GetSelectedFilter();

            appState.audioTrack.mutex().lock();
            const auto data = appState.audioTrack.data();
            appState.audioTrack.mutex().unlock();

            const auto formats = audiofiles::getCompatibleFormats(trackSampleRate);
            const auto& format = formats[formatIndex];

            if (audiofiles::writeFile(filePath, format, data, trackSampleRate)) {
            }
        }
        std::cout << "audio file write filter: "
                  << audiofiles::getWriteFilter(trackSampleRate) << std::endl;
        ifd::FileDialog::Instance().Close();
    }
}