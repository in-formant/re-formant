#include <ImFileDialog.h>

#include <iostream>
#include <shared_mutex>

#include "../audiofiles/audiofiles.h"
#include "../processing/controller/formantcontroller.h"
#include "../processing/controller/pitchcontroller.h"
#include "../processing/controller/spectrogramcontroller.h"
#include "../processing/controller/waveformcontroller.h"
#include "ui_private.h"

void reformant::ui::dockspace(AppState& appState) {
    // No need to lock track here, normally neither File> menu items
    // should be used while track is being written to.

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

    const int trackSampleRate = appState.audioTrack.sampleRate();

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::BeginDisabled(appState.audio->isCapturing() ||
                                 appState.audio->isPlaying());

            if (ImGui::MenuItem("New", "CTRL+N", nullptr)) {
                if (appState.audio->isPlaying()) appState.audio->stopPlayback();
                appState.audioTrack.reset();
                appState.spectrogramController->setTime(0);
                appState.spectrogramController->forceClear();
                appState.pitchController->forceClear();
                appState.formantController->forceClear();
                appState.waveformController->forceClear();
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

            ImGui::EndDisabled();

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
                appState.audioTrack.append(data, sampleRate);
            }
        }
        ifd::FileDialog::Instance().Close();

        // std::cout << "audio file read filter: " << audiofiles::getReadFilter() <<
        // std::endl;
    }

    if (ifd::FileDialog::Instance().IsDone("AudioFileSaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            const auto filePath = ifd::FileDialog::Instance().GetResult().string();
            const size_t formatIndex = ifd::FileDialog::Instance().GetFilterSelection();
            const auto data = appState.audioTrack.data();

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