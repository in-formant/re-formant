#ifndef REFORMANT_AUDIO_AUDIOOUTPUT_H
#define REFORMANT_AUDIO_AUDIOOUTPUT_H

#include <readerwriterqueue/readerwriterqueue.h>

#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "audiodevices.h"
#include "portaudio.h"

namespace reformant {

using namespace moodycamel;

class AudioOutput {
   public:
    using BufferCallback = std::function<bool(std::vector<float> &)>;

    AudioOutput();

    void setDevice(AudioDeviceInfo device);
    void setBufferCallback(BufferCallback callback);

    void startPlaying();
    void stopPlaying();

    bool isPlaying() const;

    void closeStream();

    double sampleRate() const;

   private:
    bool setupStream();

    static int streamCallback(const void *input, void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);

    AudioDeviceInfo m_device;

    PaDeviceIndex m_streamDevice;
    PaStream *m_stream;

    bool m_isPlaying;

    BufferCallback m_bufferCallback;
};

};  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIOOUTPUT_H