#ifndef REFORMANT_AUDIO_PORTAUDIO_BACKEND_H
#define REFORMANT_AUDIO_PORTAUDIO_BACKEND_H

#include <portaudio.h>

#include "../audiobackend.h"
#include "device.h"

namespace reformant {

class AudioBackendPortAudio : public AudioBackend {
   public:
    AudioBackendPortAudio(AudioController& controller);

    bool initialize() override;
    bool terminate() override;

    AudioBackendType type() const override;
    std::string name() const override;

    std::vector<std::weak_ptr<AudioDevice>> devices() override;

    std::weak_ptr<AudioDevice> defaultCaptureDevice() override;
    std::weak_ptr<AudioDevice> defaultPlaybackDevice() override;

   private:
    bool m_successfulPaInit;

    std::vector<std::shared_ptr<AudioDevicePortAudio>> m_devices;

    std::weak_ptr<AudioDevicePortAudio> m_defaultCaptureDevice;
    std::weak_ptr<AudioDevicePortAudio> m_defaultPlaybackDevice;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_PORTAUDIO_BACKEND_H