#ifndef REFORMANT_AUDIO_AUDIOCONTROLLER_H
#define REFORMANT_AUDIO_AUDIOCONTROLLER_H

#include <atomic>
#include <memory>
#include <vector>

#include "../readerwriterqueue/readerwriterqueue.h"
#include "audiobackend.h"

namespace reformant {

class AppState;
class AudioDevice;

class AudioController {
   public:
    AudioController(AppState &appState);

    bool initialize();
    bool terminate();

    const std::vector<std::unique_ptr<AudioBackend>> &backends();
    AudioBackend *backend(AudioBackendType type);

    bool setCaptureDevice(std::weak_ptr<AudioDevice> devicePtr);
    bool setPlaybackDevice(std::weak_ptr<AudioDevice> devicePtr);

    std::weak_ptr<AudioDevice> currentCaptureDevice();
    std::weak_ptr<AudioDevice> currentPlaybackDevice();

    bool isCapturing();
    bool startCapture();
    bool stopCapture();

    bool isPlaying();
    bool startPlayback();
    bool stopPlayback();

    // To be called from the the non-audio threads:
    size_t availCaptureFrames() const;
    size_t missingPlaybackFrames() const;
    unsigned long pullCaptureFrames(float *frm, unsigned long frmCount);
    unsigned long pushPlaybackFrames(const float *frm, unsigned long frmCount);
    void clearPlaybackFrames();

    // To be called from the audio thread:
    bool pushCaptureFrames(const float *frm, unsigned long frmCount);
    bool pullPlaybackFrames(float *frm, unsigned long frmCount);

   private:
    AppState &appState;

    std::vector<std::unique_ptr<AudioBackend>> m_backends;

    std::weak_ptr<AudioDevice> m_currentCaptureDevice;
    std::weak_ptr<AudioDevice> m_currentPlaybackDevice;

    std::atomic<bool> m_isCapturing;
    std::atomic<bool> m_isPlaying;

    moodycamel::ReaderWriterQueue<float> m_captureBuffer;
    std::atomic<bool> m_captureBufferOverflow;

    moodycamel::ReaderWriterQueue<float> m_playbackBuffer;
    std::atomic<bool> m_playbackBufferUnderrun;

    int m_playbackInitialOffset;
    int m_playbackCurrentOffset;
};

}  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIOCONTROLLER_H