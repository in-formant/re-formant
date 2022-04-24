#include "setup_audio.h"

#include "../processing/spectrogramcontroller.h"
#include "../state.h"

void reformant::setupAudio(AppState& appState) {
    appState.audioDevices.refreshHostInfo();

    // Find host api
    bool doSetDefaultHost = true;
    const int hostApiType = appState.settings.audioHostApi();
    const AudioHostApiInfo* hostApi;
    if (hostApiType >= 0) {
        hostApi = appState.audioDevices.hostApiByType(hostApiType);
        if (hostApi != nullptr) {
            appState.audioDevices.refreshDeviceInfo(hostApi->index);
            appState.ui.currentAudioHostApi = hostApi;
            doSetDefaultHost = false;
        }
    }
    if (doSetDefaultHost) {
        hostApi = appState.audioDevices.defaultHostApi();
        appState.audioDevices.refreshDeviceInfo(hostApi->index);
        appState.ui.currentAudioHostApi = hostApi;
    }

    // Check if there's a valid input device saved in the settings.
    {
        bool doSetDefaultDevice = true;

        const auto deviceName = appState.settings.inputDeviceName();
        auto device = appState.audioDevices.inputDeviceByName(deviceName);
        if (device != nullptr) {
            appState.ui.currentAudioInDevice = device;
            doSetDefaultDevice = false;
        }

        if (doSetDefaultDevice) {
            appState.ui.currentAudioInDevice =
                appState.audioDevices.inputDevice(hostApi->defaultInputDevice);
        }

        appState.audioInput.setDevice(*appState.ui.currentAudioInDevice);

        if (appState.settings.doStartRecordingOnLaunch()) {
            appState.audioInput.startRecording();
        }
    }

    // Check if there's a valid output device saved in the settings.
    {
        bool doSetDefaultDevice = true;

        const auto deviceName = appState.settings.outputDeviceName();
        auto device = appState.audioDevices.outputDeviceByName(deviceName);
        if (device != nullptr) {
            appState.ui.currentAudioOutDevice = device;
            doSetDefaultDevice = false;
        }

        if (doSetDefaultDevice) {
            appState.ui.currentAudioOutDevice =
                appState.audioDevices.outputDevice(
                    hostApi->defaultOutputDevice);
        }

        appState.audioOutput.setDevice(*appState.ui.currentAudioOutDevice);
    }

    appState.audioTrack.setSampleRate(appState.settings.trackSampleRate());

    appState.audioOutput.setBufferCallback(
        [&](std::vector<float>& buffer) -> bool {
            const int bufferLength = buffer.size();
            const int trackSamples = appState.audioTrack.sampleCount();
            const int offset = appState.spectrogramController->timeSamples();

            if (offset < trackSamples) {
                const int copyLength =
                    std::min(bufferLength, trackSamples - offset);
                const auto chunk = appState.audioTrack.data(offset, copyLength);
                std::fill(buffer.begin(), buffer.end(), 0.0);
                std::copy(chunk.begin(), chunk.begin() + copyLength,
                          buffer.begin());

                appState.spectrogramController->setTimeSamples(offset +
                                                               bufferLength);
                return true;
            } else {
                // Played past the end: stop playing.
                return false;
            }
        });
}