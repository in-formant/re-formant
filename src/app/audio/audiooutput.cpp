#include "audiooutput.h"

#include <iostream>

#include "portaudio.h"

using namespace reformant;

AudioOutput::AudioOutput()
    : m_streamDevice(paNoDevice), m_stream(nullptr), m_isPlaying(false) {}

void AudioOutput::setDevice(AudioDeviceInfo device) {
    bool restart = m_isPlaying;
    if (restart) stopPlaying();

    m_device = device;
    std::cout << "AudioOutput: device set: " << device.name << std::endl;

    if (restart) startPlaying();
}

void AudioOutput::setBufferCallback(BufferCallback callback) {
    m_bufferCallback = callback;
}

void AudioOutput::startPlaying() {
    bool isSetup = false;

    if (m_stream == nullptr) {
        isSetup = setupStream();
    } else if (m_device.index != m_streamDevice) {
        closeStream();
        isSetup = setupStream();
    } else {
        isSetup = true;
    }

    if (!isSetup) {
        std::cerr << "Audio stream is not set up for playback" << std::endl;
        return;
    }

    PaError err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
        return;
    }
    m_isPlaying = true;

    std::cout << "AudioOutput: playback started" << std::endl;
}

void AudioOutput::stopPlaying() {
    PaError err = Pa_StopStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to stop audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
    }
    m_isPlaying = false;

    std::cout << "AudioOutput: playback stopped" << std::endl;
}

bool AudioOutput::isPlaying() const { return m_isPlaying; }

bool AudioOutput::setupStream() {
    PaStreamParameters streamParameters;
    streamParameters.device = m_device.index;
    streamParameters.channelCount = 1;
    streamParameters.sampleFormat = paFloat32;
    streamParameters.suggestedLatency = m_device.defaultOutputLatency;
    streamParameters.hostApiSpecificStreamInfo = nullptr;

    // Use the highest available sample rate.
    const double sampleRate = m_device.availableSampleRates.back();

    PaError err = Pa_OpenStream(&m_stream, nullptr, &streamParameters,
                                sampleRate, paFramesPerBufferUnspecified,
                                paNoFlag, &streamCallback, this);
    if (err != paNoError) {
        std::cerr << "Failed to open audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
        return false;
    }
    m_streamDevice = m_device.index;

    std::cout << "AudioOutput: stream opened" << std::endl;
    return true;
}

void AudioOutput::closeStream() {
    if (m_stream == nullptr) {
        return;
    }

    if (m_isPlaying) {
        stopPlaying();
    }

    PaError err = Pa_CloseStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to close audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
    }
    m_streamDevice = paNoDevice;
    m_stream = nullptr;
    m_isPlaying = false;

    std::cout << "AudioOutput: stream closed" << std::endl;
}

double AudioOutput::sampleRate() const {
    // The stream uses the highest available sample rate.
    return m_device.availableSampleRates.back();
}

int AudioOutput::streamCallback(const void *input, void *voidOutput,
                                unsigned long frameCount,
                                const PaStreamCallbackTimeInfo *timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData) {
    const auto output = static_cast<float *>(voidOutput);
    auto self = static_cast<AudioOutput *>(userData);

    std::vector<float> buffer(frameCount);
    bool doContinue = self->m_bufferCallback(buffer);
    std::copy(buffer.begin(), buffer.end(), output);

    return doContinue ? paContinue : paComplete;
}
