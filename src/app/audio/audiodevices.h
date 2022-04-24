#ifndef REFORMANT_AUDIO_AUDIODEVICES_H
#define REFORMANT_AUDIO_AUDIODEVICES_H

#include <portaudio.h>

#include <string>
#include <vector>

namespace reformant {

struct AudioHostApiInfo {
    PaHostApiIndex index;
    PaHostApiTypeId type;
    const char *name;
    int deviceCount;
    PaDeviceIndex defaultInputDevice;
    PaDeviceIndex defaultOutputDevice;
};

struct AudioDeviceInfo {
    PaDeviceIndex index;
    const char *name;
    int defaultInputLatency;
    int defaultOutputLatency;
    int defaultSampleRate;
    std::vector<double> availableSampleRates;
};

struct AudioDeviceSettings {
    PaHostApiTypeId hostApi;
    const char *device;
};

class AudioDevices {
   public:
    void refreshHostInfo();
    void refreshDeviceInfo(PaHostApiIndex hostApiIndex);

    const std::vector<AudioHostApiInfo> &hostApiInfos() const;
    const std::vector<AudioDeviceInfo> &inputDeviceInfos() const;
    const std::vector<AudioDeviceInfo> &outputDeviceInfos() const;

    const AudioHostApiInfo *hostApi(PaHostApiIndex index) const;
    const AudioDeviceInfo *inputDevice(PaDeviceIndex index) const;
    const AudioDeviceInfo *outputDevice(PaDeviceIndex index) const;

    const AudioHostApiInfo *defaultHostApi() const;
    const AudioDeviceInfo *defaultInputDevice() const;
    const AudioDeviceInfo *defaultOutputDevice() const;

    const AudioHostApiInfo *hostApiByType(int hostApiTypeId) const;
    const AudioDeviceInfo *inputDeviceByName(const std::string &name) const;
    const AudioDeviceInfo *outputDeviceByName(const std::string &name) const;

    AudioDeviceSettings inputDeviceSettings(PaDeviceIndex index) const;

   private:
    std::vector<double> supportedSampleRates(PaDeviceIndex index,
                                             bool input) const;

    std::vector<AudioHostApiInfo> m_hostApiInfos;
    std::vector<AudioDeviceInfo> m_inputDevices;
    std::vector<AudioDeviceInfo> m_outputDevices;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIODEVICES_H