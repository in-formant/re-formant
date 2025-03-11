#include "audiocontroller.h"

#include <iostream>

#ifdef REFORMANT_HAS_PIPEWIRE
    #include "pipewire/backend.h"
#endif

#include "portaudio/backend.h"

using namespace reformant;

AudioController::AudioController() : m_captureBuffer(16384), m_playbackBuffer(16384) {
#ifdef REFORMANT_HAS_PIPEWIRE
    m_backends.push_back(std::make_unique<AudioBackendPipewire>(*this));
#endif
    m_backends.push_back(std::make_unique<AudioBackendPortAudio>(*this));
}

bool AudioController::initialize() {
    bool atLeastOne = false;

    for (auto iter = m_backends.begin(); iter != m_backends.end(); ++iter) {
        if ((*iter)->initialize()) {
            atLeastOne = true;
        } else {
            // If it failed to initialise, log the failure and
            // remove it from the list of backends.
            std::cout << "AudioController::initialize: audio backend " << (*iter)->name()
                      << " failed to initialize" << std::endl;
            iter = m_backends.erase(iter);
        }
    }

    return atLeastOne;
}

bool AudioController::terminate() {
    bool allOfThem = true;

    for (auto& backend : m_backends) {
        if (!backend->terminate()) {
            std::cout << "AudioController::terminate: audio backend " << backend->name()
                      << " failed to terminate" << std::endl;
            allOfThem = false;
        }
    }

    return allOfThem;
}

const std::vector<std::unique_ptr<AudioBackend>>& AudioController::backends() {
    return m_backends;
}

AudioBackend* AudioController::backend(AudioBackendType type) {
    for (auto& backend : m_backends) {
        if (backend->type() == type) {
            return backend.get();
        }
    }
    std::cout << "AudioController::backend: backend not found" << std::endl;
    return nullptr;
}

bool AudioController::setCaptureDevice(std::weak_ptr<AudioDevice> devicePtr) {
    auto device = devicePtr.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::setCaptureDevice: device expired" << std::endl;
        return false;
    }
    if (!device->canCapture()) {
        std::cout << "AudioController::setCaptureDevice: device " << device->name()
                  << " is not a capture device" << std::endl;
        return false;
    }
    // Restart capture if there is already a capture device active.
    bool resumeCapture = isCapturing();
    if (resumeCapture) {
        if (!stopCapture()) {
            std::cout << "AudioController::setCaptureDevice: "
                         "previous device could not be stopped"
                      << std::endl;
        }
    }
    m_currentCaptureDevice = device;
    if (resumeCapture) {
        if (!startCapture()) {
            std::cout << "AudioController::setCaptureDevice: "
                         "new device could not be started"
                      << std::endl;
        }
    }
    return true;
}

bool AudioController::setPlaybackDevice(std::weak_ptr<AudioDevice> devicePtr) {
    auto device = devicePtr.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::setPlaybackDevice: device expired" << std::endl;
        return false;
    }
    if (!device->canPlayback()) {
        std::cout << "AudioController::setPlaybackDevice: device " << device->name()
                  << " is not a playback device" << std::endl;
        return false;
    }
    // Restart capture if there is already a capture device active.
    bool resumePlayback = isPlaying();
    if (resumePlayback) {
        if (!stopPlayback()) {
            std::cout << "AudioController::setPlaybackDevice: "
                         "previous device could not be stopped"
                      << std::endl;
        }
    }
    m_currentPlaybackDevice = device;
    if (resumePlayback) {
        if (!startPlayback()) {
            std::cout << "AudioController::setPlaybackDevice: "
                         "new device could not be started"
                      << std::endl;
        }
    }
    return true;
}

std::weak_ptr<AudioDevice> AudioController::currentCaptureDevice() {
    return m_currentCaptureDevice;
}

std::weak_ptr<AudioDevice> AudioController::currentPlaybackDevice() {
    return m_currentPlaybackDevice;
}

bool AudioController::isCapturing() {
    auto device = m_currentCaptureDevice.lock();
    return device && device->isValid() && device->isCapturing();
}

bool AudioController::startCapture() {
    auto device = m_currentCaptureDevice.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::startCapture: device expired" << std::endl;
        return false;
    }
    if (!device->canCapture()) {
        std::cout << "AudioController::startCapture: device " << device->name()
                  << " is not a capture device" << std::endl;
        return false;
    }
    if (device->isCapturing()) {
        std::cout << "AudioController::startCapture: already capturing" << std::endl;
        return false;
    }
    return device->startCapture();
}

bool AudioController::stopCapture() {
    auto device = m_currentCaptureDevice.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::stopCapture: device expired" << std::endl;
        return false;
    }
    if (!device->canCapture()) {
        std::cout << "AudioController::stopCapture: device " << device->name()
                  << " is not a capture device" << std::endl;
        return false;
    }
    if (!device->isCapturing()) {
        std::cout << "AudioController::stopCapture: not capturing" << std::endl;
        return false;
    }
    return device->stopCapture();
}

bool AudioController::isPlaying() {
    auto device = m_currentPlaybackDevice.lock();
    return device && device->isValid() && device->isPlaying();
}

bool AudioController::startPlayback() {
    auto device = m_currentPlaybackDevice.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::startPlayback: device expired" << std::endl;
        return false;
    }
    if (!device->canPlayback()) {
        std::cout << "AudioController::startPlayback: device " << device->name()
                  << " is not a playback device" << std::endl;
        return false;
    }
    if (!device->isPlaying()) {
        std::cout << "AudioController::startPlayback: already playing" << std::endl;
        return false;
    }
    return device->startPlayback();
}

bool AudioController::stopPlayback() {
    auto device = m_currentPlaybackDevice.lock();
    if (!device || !device->isValid()) {
        std::cout << "AudioController::stopPlayback: device expired" << std::endl;
        return false;
    }
    if (!device->canPlayback()) {
        std::cout << "AudioController::stopPlayback: device " << device->name()
                  << " is not a playback device" << std::endl;
        return false;
    }
    if (!device->isPlaying()) {
        std::cout << "AudioController::stopPlayback: not playing" << std::endl;
        return false;
    }
    return device->stopPlayback();
}

// To be called from the the non-audio threads:

unsigned long AudioController::pullCaptureFrames(float* frm, unsigned long frmCount) {
    // notify if there was overflow
    if (m_captureBufferOverflow.load()) {
        m_captureBufferOverflow.store(false);
        std::cout << "AudioController: capture buffer overflow" << std::endl;
    }
    for (unsigned long i = 0; i < frmCount; ++i) {
        if (!m_captureBuffer.try_dequeue(frm[i])) {
            return i;
        }
    }
    return frmCount;
}

unsigned long AudioController::pushPlaybackFrames(const float* frm,
                                                  unsigned long frmCount) {
    // notify if there was buffer underrun
    if (m_playbackBufferUnderrun.load()) {
        m_playbackBufferUnderrun.store(false);
        std::cout << "AudioController: playback buffer underrun" << std::endl;
    }
    for (unsigned long i = 0; i < frmCount; ++i) {
        if (!m_playbackBuffer.try_enqueue(frm[i])) {
            return i;
        }
    }
    return frmCount;
}

// To be called from the audio thread:

void AudioController::pushCaptureFrames(const float* frm, unsigned long frmCount) {
    for (unsigned long i = 0; i < frmCount; ++i) {
        if (!m_captureBuffer.try_enqueue(frm[i])) {
            m_captureBufferOverflow.store(true);
            break;
        }
    }
}

void AudioController::pullPlaybackFrames(float* frm, unsigned long frmCount) {
    for (unsigned long i = 0; i < frmCount; ++i) {
        if (!m_playbackBuffer.try_dequeue(frm[i])) {
            m_playbackBufferUnderrun.store(true);
            break;
        }
    }
}
