#ifndef REFORMANT_AUDIO_AUDIODEVICE_H
#define REFORMANT_AUDIO_AUDIODEVICE_H

#include <string>

namespace reformant {

class AudioController;

class AudioDevice {
   public:
    virtual std::string name() const = 0;

    // Returns false if the device doesn't exist or is invalid.
    virtual bool isValid() = 0;

    virtual double sampleRate() = 0;

    virtual bool canCapture() = 0;
    virtual bool isCapturing() = 0;

    virtual bool startCapture() = 0;
    virtual bool stopCapture() = 0;

    virtual bool canPlayback() = 0;
    virtual bool isPlaying() = 0;

    virtual bool startPlayback() = 0;
    virtual bool stopPlayback() = 0;

   protected:
    AudioDevice(AudioController& controller) : m_controller(controller) {}
    virtual ~AudioDevice() {}

    virtual bool pushCaptureFrames(const float* frm, unsigned long frmCount) = 0;
    virtual bool pullPlaybackFrames(float* frm, unsigned long frmCount) = 0;

    AudioController& m_controller;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIODEVICE_H