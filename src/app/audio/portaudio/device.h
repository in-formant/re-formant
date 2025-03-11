#ifndef REFORMANT_AUDIO_PORTAUDIO_DEVICE_H
#define REFORMANT_AUDIO_PORTAUDIO_DEVICE_H

#include <portaudio.h>

#include "../audiodevice.h"

namespace reformant {

class AudioDevicePortAudio : public AudioDevice {
   public:
    AudioDevicePortAudio(AudioController& controller, PaDeviceIndex deviceIndex);

    std::string name() const override;

    // Returns false if the device doesn't exist or is invalid.
    bool isValid() override;

    double sampleRate() override;

    bool canCapture() override;
    bool isCapturing() override;

    bool startCapture() override;
    bool stopCapture() override;

    bool canPlayback() override;
    bool isPlaying() override;

    bool startPlayback() override;
    bool stopPlayback() override;

    // - impl

    void invalidate();
    void pushCaptureFrames(const float* frm, unsigned long frmCount) override;
    void pullPlaybackFrames(float* frm, unsigned long frmCount) override;

   private:
    bool m_isValid;
    PaDeviceIndex m_deviceIndex;
    const PaDeviceInfo* m_deviceInfo;
    std::string m_name;

    bool m_isCapturing;
    PaStream* m_captureStream;

    bool m_isPlaying;
    PaStream* m_playbackStream;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_PORTAUDIO_DEVICE_H