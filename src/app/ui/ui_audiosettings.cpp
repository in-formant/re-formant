#include <array>

#include "../memusage.h"
#include "../processing/controller/pitchcontroller.h"
#include "../processing/controller/spectrogramcontroller.h"
#include "ui_private.h"

void reformant::ui::audioSettings(AppState& appState) {
    if (ImGui::Begin("Audio settings", &appState.ui.showAudioSettings)) {
        const auto& backends = appState.audio.backends();

        std::string backendName =
            appState.audio.backend(appState.ui.audioBackend)->name();

        auto captureDevice = appState.audio.currentCaptureDevice().lock();
        std::string captureDeviceName = captureDevice ? captureDevice->name() : "";

        auto playbackDevice = appState.audio.currentPlaybackDevice().lock();
        std::string playbackDeviceName = playbackDevice ? playbackDevice->name() : "";

        if (ImGui::BeginCombo("Backend", backendName.c_str())) {
            for (int i = 0; i < backends.size(); ++i) {
                const auto& backend = backends[i];
                const bool isSelected = (appState.ui.audioBackend == backend->type());

                if (ImGui::Selectable(backend->name().c_str(), isSelected)) {
                    auto defaultCaptureDevice = backend->defaultCaptureDevice().lock();
                    auto defaultPlaybackDevice = backend->defaultPlaybackDevice().lock();

                    appState.audio.setCaptureDevice(backend->defaultCaptureDevice());
                    appState.audio.setPlaybackDevice(backend->defaultPlaybackDevice());
                    appState.audioOutputResampler.setRate(
                        appState.audioTrack.sampleRate(),
                        defaultCaptureDevice->sampleRate());
                    appState.ui.audioBackend = backend->type();
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        const auto& selectedBackend = appState.audio.backend(appState.ui.audioBackend);

        if (ImGui::BeginCombo("Capture device", captureDeviceName.c_str())) {
            for (auto& devicePtr : selectedBackend->devices()) {
                auto device = devicePtr.lock();
                if (device && device->isValid() && device->canCapture()) {
                    bool isSelected = (captureDeviceName == device->name());

                    if (ImGui::Selectable(device->name().c_str(), isSelected)) {
                        appState.audio.setCaptureDevice(device);
                        appState.audioOutputResampler.setRate(
                            appState.audioTrack.sampleRate(), device->sampleRate());
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Playback device", playbackDeviceName.c_str())) {
            for (auto& devicePtr : selectedBackend->devices()) {
                auto device = devicePtr.lock();
                if (device && device->isValid() && device->canPlayback()) {
                    bool isSelected = (playbackDeviceName == device->name());

                    if (ImGui::Selectable(device->name().c_str(), isSelected)) {
                        appState.audio.setPlaybackDevice(device);
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        bool autoStartRecord = appState.settings.doStartRecordingOnLaunch();
        if (ImGui::Checkbox("Start recording on launch", &autoStartRecord)) {
            appState.settings.setStartRecordingOnLaunch(autoStartRecord);
        }

        bool enableNoiseReduction = appState.settings.doNoiseReduction();
        if (ImGui::Checkbox("Enable noise reduction", &enableNoiseReduction)) {
            std::lock_guard trackGuard(appState.audioTrack.mutex());
            appState.audioTrack.setDenoising(enableNoiseReduction);
            appState.settings.setNoiseReduction(enableNoiseReduction);
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
                    bool wasPlaying = appState.audio.isPlaying();
                    double time = appState.spectrogramController->time();
                    if (wasPlaying) appState.audio.stopPlayback();
                    appState.audioTrack.setSampleRate(sampleRate);
                    appState.settings.setTrackSampleRate(sampleRate);
                    appState.spectrogramController->forceClear();
                    if (wasPlaying) appState.audio.startPlayback();
                    appState.spectrogramController->setTime(time);
                }
            }

            ImGui::EndCombo();
        }

        const int currentFftLength = (int)appState.spectrogramController->fftLength();

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

        if (ImGui::InputFloat("Max spectrogram memory usage", &maxSpecMemoryMb, 1.0f,
                              16.0f, "%.0f MB")) {
            appState.spectrogramController->setMaxMemoryMemo((uint64_t)maxSpecMemoryMb *
                                                             1024_u64 * 1024_u64);
        }

        ImGui::Text("(approximately %.2f seconds before forced refresh)",
                    0.9 * appState.spectrogramController->approxMemoCapacityInSeconds());
    }
    ImGui::End();

    appState.settings.setShowAudioSettings(appState.ui.showAudioSettings);
}