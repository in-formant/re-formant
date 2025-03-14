#include "backend.h"

#include <portaudio.h>

#include <iostream>

using namespace reformant;

AudioBackendPortAudio::AudioBackendPortAudio(AudioController& controller)
    : AudioBackend(controller), m_successfulPaInit(false) {}

bool AudioBackendPortAudio::initialize() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "PortAudio::initialize: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    m_successfulPaInit = true;

    // Enumerate devices.
    #if defined(__linux)
    PaHostApiIndex hostApi = Pa_HostApiTypeIdToHostApiIndex(paJACK);
    #elif defined(_WIN32)
    PaHostApiIndex hostApi = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
    #endif
    const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(hostApi);
    int hostDefInDev = hostInfo->defaultInputDevice;
    int hostDefOutDev = hostInfo->defaultOutputDevice;
    for (int i = 0; i < hostInfo->deviceCount; ++i) {
        PaDeviceIndex deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(hostApi, i);
        m_devices.push_back(
            std::make_unique<AudioDevicePortAudio>(m_controller, deviceIndex));
        if (i == hostDefInDev) {
            m_defaultCaptureDevice = m_devices.back();
        }
        if (i == hostDefOutDev) {
            m_defaultPlaybackDevice = m_devices.back();
        }
    }

    return true;
}

bool AudioBackendPortAudio::terminate() {
    if (!m_successfulPaInit) {
        std::cout << "PortAudio::terminate: was not initialized in the first place"
                  << std::endl;
        return false;
    }

    // Flip the invalid bit for all devices.
    for (auto& device : m_devices) {
        if (device->isCapturing()) device->stopCapture();
        if (device->isPlaying()) device->stopPlayback();
        device->invalidate();
    }

    m_devices.clear();

    PaError err = Pa_Terminate();
    if (err != paNoError) {
        std::cout << "PortAudio::terminate: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    return true;
}

AudioBackendType AudioBackendPortAudio::type() const { return AudioBackend_PortAudio; }

std::string AudioBackendPortAudio::name() const { return "PortAudio"; }

std::vector<std::weak_ptr<AudioDevice>> AudioBackendPortAudio::devices() {
    std::vector<std::weak_ptr<AudioDevice>> devicePtrs(m_devices.size());
    std::copy(m_devices.begin(), m_devices.end(), devicePtrs.begin());
    return devicePtrs;
}

std::weak_ptr<AudioDevice> AudioBackendPortAudio::defaultCaptureDevice() {
    return m_defaultCaptureDevice;
}

std::weak_ptr<AudioDevice> AudioBackendPortAudio::defaultPlaybackDevice() {
    return m_defaultPlaybackDevice;
}