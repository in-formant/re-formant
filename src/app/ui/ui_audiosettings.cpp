#include <array>

#include "../memusage.h"
#include "../processing/pitchcontroller.h"
#include "../processing/spectrogramcontroller.h"
#include "ui_private.h"

void reformant::ui::audioSettings(AppState& appState) {
    if (ImGui::Begin("Audio settings", &appState.ui.showAudioSettings)) {
        if (ImGui::BeginCombo("Host API",
                              appState.ui.currentAudioHostApi->name)) {
            const auto& hostApis = appState.audioDevices.hostApiInfos();

            for (int i = 0; i < hostApis.size(); ++i) {
                const auto& hostApi = hostApis[i];
                const bool isSelected =
                    (appState.ui.currentAudioHostApi->index == hostApi.index);

                if (ImGui::Selectable(hostApi.name, isSelected)) {
                    appState.ui.currentAudioHostApi = &hostApi;
                    appState.audioDevices.refreshDeviceInfo(hostApi.index);

                    // Set current device to default host API device.
                    const auto& defaultInputDevice =
                        appState.audioDevices.inputDevice(
                            hostApi.defaultInputDevice);
                    const auto& defaultOutputDevice =
                        appState.audioDevices.outputDevice(
                            hostApi.defaultOutputDevice);

                    appState.ui.currentAudioInDevice = defaultInputDevice;
                    appState.audioInput.setDevice(*defaultInputDevice);

                    appState.ui.currentAudioOutDevice = defaultOutputDevice;
                    appState.audioOutput.setDevice(*defaultOutputDevice);

                    appState.settings.setAudioHostApi(hostApi.type);
                    appState.settings.setInputDeviceName(
                        defaultInputDevice->name);
                    appState.settings.setOutputDeviceName(
                        defaultOutputDevice->name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Input device",
                              appState.ui.currentAudioInDevice->name)) {
            const auto& devices = appState.audioDevices.inputDeviceInfos();

            for (int i = 0; i < devices.size(); ++i) {
                const auto& device = devices[i];
                const bool isSelected =
                    (appState.ui.currentAudioInDevice->index == device.index);

                if (ImGui::Selectable(device.name, isSelected)) {
                    appState.ui.currentAudioInDevice = &device;
                    appState.audioInput.setDevice(device);

                    appState.settings.setInputDeviceName(device.name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Output device",
                              appState.ui.currentAudioOutDevice->name)) {
            const auto& devices = appState.audioDevices.outputDeviceInfos();

            for (int i = 0; i < devices.size(); ++i) {
                const auto& device = devices[i];
                const bool isSelected =
                    (appState.ui.currentAudioOutDevice->index == device.index);

                if (ImGui::Selectable(device.name, isSelected)) {
                    appState.ui.currentAudioOutDevice = &device;
                    appState.audioOutput.setDevice(device);

                    appState.settings.setOutputDeviceName(device.name);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        bool autoStartRecord = appState.settings.doStartRecordingOnLaunch();
        if (ImGui::Checkbox("Start recording on launch", &autoStartRecord)) {
            appState.settings.setStartRecordingOnLaunch(autoStartRecord);
        }

        const int currentSampleRate = (int)appState.audioTrack.sampleRate();

        if (ImGui::BeginCombo("Recording track sample rate",
                              std::to_string(currentSampleRate).c_str())) {
            std::array trackSampleRates(std::to_array({
                8000,
                11025,
                16000,
                22050,
                32000,
                44100,
                48000,
            }));

            for (const int sampleRate : trackSampleRates) {
                const bool isSelected = (currentSampleRate == sampleRate);
                const auto name = std::to_string(sampleRate);

                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    std::lock_guard trackGuard(appState.audioTrack.mutex());
                    appState.audioTrack.setSampleRate(sampleRate);
                    appState.settings.setTrackSampleRate(sampleRate);
                    appState.spectrogramController->forceClear();
                    appState.pitchController->forceClear();
                }
            }

            ImGui::EndCombo();
        }

        const int currentFftLength =
            (int)appState.spectrogramController->fftLength();

        if (ImGui::BeginCombo("Spectrogram FFT length",
                              std::to_string(currentFftLength).c_str())) {
            std::array nffts = {
                128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
            };

            for (const int nfft : nffts) {
                const bool isSelected = (currentFftLength == nfft);
                const auto name = std::to_string(nfft);

                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    appState.spectrogramController->setFftLength(nfft);
                    appState.settings.setFftLength(nfft);
                }
            }

            ImGui::EndCombo();
        }

        float maxSpecMemoryMb =
            appState.spectrogramController->maxMemoryMemo() / 1024.0f / 1024.0f;

        if (ImGui::InputFloat("Max spectrogram memory usage", &maxSpecMemoryMb,
                              1.0f, 16.0f, "%.0f MB")) {
            appState.spectrogramController->setMaxMemoryMemo(
                (uint64_t)maxSpecMemoryMb * 1024_u64 * 1024_u64);
        }

        ImGui::Text(
            "(approximately %.2f seconds before forced refresh)",
            0.9 *
                appState.spectrogramController->approxMemoCapacityInSeconds());
    }
    ImGui::End();

    appState.settings.setShowAudioSettings(appState.ui.showAudioSettings);
}