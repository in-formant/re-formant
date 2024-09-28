#include "audiodevices.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "portaudio.h"

using namespace reformant;

void AudioDevices::refreshHostInfo() {
    const int count = Pa_GetHostApiCount();

    m_hostApiInfos.clear();

    for (PaHostApiIndex index = 0; index < count; ++index) {
        const auto paInfo = Pa_GetHostApiInfo(index);

        AudioHostApiInfo info;
        info.index = index;
        info.type = paInfo->type;
        info.name = paInfo->name;
        info.deviceCount = paInfo->deviceCount;
        info.defaultInputDevice = paInfo->defaultInputDevice;
        info.defaultOutputDevice = paInfo->defaultOutputDevice;

        m_hostApiInfos.push_back(info);
    }
}

void AudioDevices::refreshDeviceInfo(int hostApiIndex) {
    const auto hostInfo = hostApi(hostApiIndex);

    m_inputDevices.clear();
    m_outputDevices.clear();

    for (PaDeviceIndex i = 0; i < hostInfo->deviceCount; ++i) {
        const int index =
            Pa_HostApiDeviceIndexToDeviceIndex(hostInfo->index, i);
        const auto paInfo = Pa_GetDeviceInfo(index);

        AudioDeviceInfo info;
        info.index = index;
        info.name = paInfo->name;
        info.defaultInputLatency = paInfo->defaultLowInputLatency;
        info.defaultOutputLatency = paInfo->defaultLowOutputLatency;
        info.defaultSampleRate = paInfo->defaultSampleRate;

        if (paInfo->maxInputChannels > 0) {
            info.availableSampleRates = supportedSampleRates(index, true);
            m_inputDevices.push_back(info);
        }

        if (paInfo->maxOutputChannels > 0) {
            info.availableSampleRates = supportedSampleRates(index, false);
            m_outputDevices.push_back(info);
        }
    }
}

std::vector<double> AudioDevices::supportedSampleRates(PaDeviceIndex index,
                                                       bool input) const {
    constexpr double standardSampleRates[] = {
        8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0,
        24000.0, 32000.0, 44100.0, 48000.0, -1};

    PaStreamParameters parameters;
    parameters.device = index;
    parameters.channelCount = 1;
    parameters.sampleFormat = paInt16;
    parameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    parameters.hostApiSpecificStreamInfo = NULL;

    std::vector<double> sampleRates;

    for (int i = 0; standardSampleRates[i] > 0; ++i) {
        const double sampleRate = standardSampleRates[i];

        PaError err;
        if (input) {
            err = Pa_IsFormatSupported(&parameters, nullptr, sampleRate);
        } else {
            err = Pa_IsFormatSupported(nullptr, &parameters, sampleRate);
        }

        if (err == paFormatIsSupported) {
            sampleRates.push_back(sampleRate);
        }
    }

    return sampleRates;
}

const std::vector<AudioHostApiInfo>& AudioDevices::hostApiInfos() const {
    return m_hostApiInfos;
}

const std::vector<AudioDeviceInfo>& AudioDevices::inputDeviceInfos() const {
    return m_inputDevices;
}

const std::vector<AudioDeviceInfo>& AudioDevices::outputDeviceInfos() const {
    return m_outputDevices;
}

const AudioHostApiInfo* AudioDevices::hostApi(PaHostApiIndex index) const {
    for (const auto& info : m_hostApiInfos) {
        if (info.index == index) {
            return &info;
        }
    }
    throw std::domain_error("Host API info not present");
}

const AudioDeviceInfo* AudioDevices::inputDevice(PaDeviceIndex index) const {
    for (const auto& info : m_inputDevices) {
        if (info.index == index) {
            return &info;
        }
    }
    throw std::domain_error("Input device info not present");
}

const AudioDeviceInfo* AudioDevices::outputDevice(PaDeviceIndex index) const {
    for (const auto& info : m_outputDevices) {
        if (info.index == index) {
            return &info;
        }
    }
    throw std::domain_error("Output device info not present");
}

const AudioHostApiInfo* AudioDevices::defaultHostApi() const {
    #ifdef _WIN32
    return hostApiByType(paWASAPI);
    #else
    return hostApi(Pa_GetDefaultHostApi());
    #endif
}

const AudioDeviceInfo* AudioDevices::defaultInputDevice() const {
    return inputDevice(Pa_GetDefaultInputDevice());
}

const AudioDeviceInfo* AudioDevices::defaultOutputDevice() const {
    return outputDevice(Pa_GetDefaultOutputDevice());
}

const AudioHostApiInfo* AudioDevices::hostApiByType(int type) const {
    for (const auto& info : m_hostApiInfos) {
        if (info.type == type) {
            return &info;
        }
    }
    return nullptr;
}

const AudioDeviceInfo* AudioDevices::inputDeviceByName(
    const std::string& name) const {
    for (const auto& info : m_inputDevices) {
        if (!strcmp(info.name, name.c_str())) {
            return &info;
        }
    }
    return nullptr;
}

const AudioDeviceInfo* AudioDevices::outputDeviceByName(
    const std::string& name) const {
    for (const auto& info : m_outputDevices) {
        if (!strcmp(info.name, name.c_str())) {
            return &info;
        }
    }
    return nullptr;
}

AudioDeviceSettings AudioDevices::inputDeviceSettings(
    PaDeviceIndex index) const {
    const auto deviceInfo = Pa_GetDeviceInfo(index);
    const auto hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

    AudioDeviceSettings settings;
    settings.hostApi = hostApiInfo->type;
    settings.device = deviceInfo->name;
    return settings;
}