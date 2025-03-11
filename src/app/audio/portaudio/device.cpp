#include "device.h"

#include <portaudio.h>

#include <iostream>

#include "../audiocontroller.h"

using namespace reformant;

static int captureCallback(const void *input, void *output, unsigned long frameCount,
                           const PaStreamCallbackTimeInfo *timeInfo,
                           PaStreamCallbackFlags statusFlags, void *userData);

static int playbackCallback(const void *input, void *output, unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags, void *userData);

AudioDevicePortAudio::AudioDevicePortAudio(AudioController &controller,
                                           PaDeviceIndex deviceIndex)
    : AudioDevice(controller),
      m_isValid(true),
      m_deviceIndex(deviceIndex),
      m_deviceInfo(Pa_GetDeviceInfo(deviceIndex)),
      m_name(m_deviceInfo->name),
      m_isCapturing(false),
      m_isPlaying(false) {}

std::string AudioDevicePortAudio::name() const { return m_name; }

bool AudioDevicePortAudio::isValid() { return m_isValid; }

double AudioDevicePortAudio::sampleRate() { return m_deviceInfo->defaultSampleRate; }

bool AudioDevicePortAudio::canCapture() { return m_deviceInfo->maxInputChannels > 0; }
bool AudioDevicePortAudio::isCapturing() { return m_isCapturing; }

bool AudioDevicePortAudio::startCapture() {
    if (!canCapture()) {
        std::cout << "AudioDevicePortAudio::startCapture: device is not a capture device"
                  << std::endl;
        return false;
    }

    PaStreamParameters params;
    params.channelCount = 1;
    params.device = m_deviceIndex;
    params.hostApiSpecificStreamInfo = nullptr;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = m_deviceInfo->defaultLowInputLatency;

    PaError err =
        Pa_OpenStream(&m_captureStream, &params, nullptr, m_deviceInfo->defaultSampleRate,
                      1024, paNoFlag, &::captureCallback, this);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::startCapture: " << Pa_GetErrorText(err)
                  << std::endl;
        return false;
    }

    m_isCapturing = true;

    err = Pa_StartStream(m_captureStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::startCapture: " << Pa_GetErrorText(err)
                  << std::endl;
        m_isCapturing = false;
        return false;
    }
    return true;
}

bool AudioDevicePortAudio::stopCapture() {
    m_isCapturing = false;

    PaError err = Pa_StopStream(m_captureStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::stopCapture: " << Pa_GetErrorText(err)
                  << std::endl;
    }

    err = Pa_CloseStream(m_captureStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::stopCapture: " << Pa_GetErrorText(err)
                  << std::endl;
    }

    return true;
}

bool AudioDevicePortAudio::canPlayback() { return m_deviceInfo->maxOutputChannels > 0; }

bool AudioDevicePortAudio::isPlaying() { return m_isPlaying; }

bool AudioDevicePortAudio::startPlayback() {
    if (!canPlayback()) {
        std::cout
            << "AudioDevicePortAudio::startPlayback: device is not a playback device"
            << std::endl;
        return false;
    }

    PaStreamParameters params;
    params.channelCount = 1;
    params.device = m_deviceIndex;
    params.hostApiSpecificStreamInfo = nullptr;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = m_deviceInfo->defaultLowOutputLatency;

    PaError err = Pa_OpenStream(&m_playbackStream, nullptr, &params,
                                m_deviceInfo->defaultSampleRate, 1024, paNoFlag,
                                &::playbackCallback, this);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::startPlayback: " << Pa_GetErrorText(err)
                  << std::endl;
        return false;
    }

    m_isPlaying = true;

    err = Pa_StartStream(m_playbackStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::startPlayback: " << Pa_GetErrorText(err)
                  << std::endl;
        m_isPlaying = false;
        return false;
    }
    return true;
}

bool AudioDevicePortAudio::stopPlayback() {
    m_isPlaying = false;

    PaError err = Pa_StopStream(m_playbackStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::stopPlayback: " << Pa_GetErrorText(err)
                  << std::endl;
    }

    err = Pa_CloseStream(m_playbackStream);
    if (err != paNoError) {
        std::cout << "AudioDevicePortAudio::stopPlayback: " << Pa_GetErrorText(err)
                  << std::endl;
    }

    return true;
}

// - impl

void AudioDevicePortAudio::invalidate() { m_isValid = false; }

void AudioDevicePortAudio::pushCaptureFrames(const float *frm, unsigned long frmCount) {
    m_controller.pushCaptureFrames(frm, frmCount);
}

void AudioDevicePortAudio::pullPlaybackFrames(float *frm, unsigned long frmCount) {
    m_controller.pullPlaybackFrames(frm, frmCount);
}

static int captureCallback(const void *input, void *output, unsigned long frameCount,
                           const PaStreamCallbackTimeInfo *timeInfo,
                           PaStreamCallbackFlags statusFlags, void *userData) {
    (void)output;
    (void)timeInfo;
    (void)statusFlags;

    auto that = static_cast<AudioDevicePortAudio *>(userData);

    that->pushCaptureFrames(static_cast<const float *>(input), frameCount);

    return that->isCapturing() ? paContinue : paComplete;
}

static int playbackCallback(const void *input, void *output, unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags, void *userData) {
    (void)output;
    (void)timeInfo;
    (void)statusFlags;

    auto that = static_cast<AudioDevicePortAudio *>(userData);

    that->pullPlaybackFrames(static_cast<float *>(output), frameCount);

    return that->isPlaying() ? paContinue : paComplete;
}
