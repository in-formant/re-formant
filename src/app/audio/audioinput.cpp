#include "audioinput.h"

#include <iostream>

#include "portaudio.h"

using namespace reformant;

AudioInput::AudioInput()
    : m_streamDevice(paNoDevice),
      m_stream(nullptr),
      m_isRecording(false),
      m_bufferQueue(30) {}

void AudioInput::setDevice(AudioDeviceInfo device) {
    bool restart = m_isRecording;
    if (restart) stopRecording();

    m_device = device;
    std::cout << "AudioInput: device set: " << device.name << std::endl;

    if (restart) startRecording();
}

void AudioInput::setBufferCallback(BufferCallback callback) {
    m_bufferCallback = callback;
}

void AudioInput::startRecording() {
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
        std::cerr << "Audio stream is not set up for recording" << std::endl;
        return;
    }

    PaError err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
        return;
    }
    m_isRecording = true;

    std::cout << "AudioInput: recording started" << std::endl;
}

void AudioInput::stopRecording() {
    PaError err = Pa_StopStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to stop audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
    }
    m_isRecording = false;

    std::cout << "AudioInput: recording stopped" << std::endl;
}

bool AudioInput::isRecording() const { return m_isRecording; }

void AudioInput::retrieveBuffers() {
    do {
        const auto buffer = m_bufferQueue.peek();
        if (buffer == nullptr) {
            break;
        }
        m_bufferCallback(*buffer);
        m_bufferQueue.pop();
    } while (true);
}

bool AudioInput::setupStream() {
    PaStreamParameters streamParameters;
    streamParameters.device = m_device.index;
    streamParameters.channelCount = 1;
    streamParameters.sampleFormat = paFloat32;
    streamParameters.suggestedLatency = m_device.defaultInputLatency;
    streamParameters.hostApiSpecificStreamInfo = nullptr;

    // Use the highest available sample rate.
    const double sampleRate = m_device.availableSampleRates.back();

    PaError err = Pa_OpenStream(&m_stream, &streamParameters, nullptr,
                                sampleRate, paFramesPerBufferUnspecified,
                                paNoFlag, &streamCallback, this);
    if (err != paNoError) {
        std::cerr << "Failed to open audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
        return false;
    }
    m_streamDevice = m_device.index;

    std::cout << "AudioInput: stream opened" << std::endl;
    return true;
}

void AudioInput::closeStream() {
    if (m_stream == nullptr) {
        return;
    }

    if (m_isRecording) {
        stopRecording();
    }

    PaError err = Pa_CloseStream(m_stream);
    if (err != paNoError) {
        std::cerr << "Failed to close audio stream: " << Pa_GetErrorText(err)
                  << std::endl;
    }
    m_streamDevice = paNoDevice;
    m_stream = nullptr;
    m_isRecording = false;

    std::cout << "AudioInput: stream closed" << std::endl;
}

double AudioInput::sampleRate() const {
    // The stream us0es the highest available sample rate.
    return m_device.availableSampleRates.back();
}

int AudioInput::streamCallback(const void *voidInput, void *output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo *timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData) {
    const auto input = static_cast<const float *>(voidInput);
    auto self = static_cast<AudioInput *>(userData);

    std::vector<float> buffer(frameCount);
    std::copy(input, input + frameCount, buffer.begin());

    if (!self->m_bufferQueue.try_enqueue(std::move(buffer))) {
        // Queue overflow
        std::cout << "buffer queue overflow" << std::endl;
    }

    return paContinue;
}
