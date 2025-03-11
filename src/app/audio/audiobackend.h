#ifndef REFORMANT_AUDIO_AUDIOBACKEND_H
#define REFORMANT_AUDIO_AUDIOBACKEND_H

#include <memory>
#include <string>
#include <vector>

#if REFORMANT_HAS_PIPEWIRE
    #include "pipewire/pipewire.h"
#endif

#include "audiodevice.h"

namespace reformant {

enum AudioBackendType {
    AudioBackend_PipeWire = 0,
    AudioBackend_PortAudio,
    AudioBackend_Count,
};

class AudioController;
class AudioDevice;

class AudioBackend {
   public:
    virtual ~AudioBackend() {}

    virtual bool initialize() = 0;
    virtual bool terminate() = 0;

    virtual AudioBackendType type() const = 0;
    virtual std::string name() const = 0;

    virtual std::vector<std::weak_ptr<AudioDevice>> devices() = 0;

    virtual std::weak_ptr<AudioDevice> defaultCaptureDevice() = 0;
    virtual std::weak_ptr<AudioDevice> defaultPlaybackDevice() = 0;

   protected:
    AudioBackend(AudioController& controller) : m_controller(controller) {}

    AudioController& m_controller;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIOBACKEND_H