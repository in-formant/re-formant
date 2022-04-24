#ifndef REFORMANT_AUDIO_AUDIOINPUT_H
#define REFORMANT_AUDIO_AUDIOINPUT_H

#include <readerwriterqueue/readerwriterqueue.h>

#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "audiodevices.h"
#include "portaudio.h"

namespace reformant {

using namespace moodycamel;

class AudioInput {
   public:
    using BufferCallback = std::function<void(const std::vector<float> &)>;

    AudioInput();

    void setDevice(AudioDeviceInfo device);
    void setBufferCallback(BufferCallback callback);

    void startRecording();
    void stopRecording();

    bool isRecording() const;

    void closeStream();

    void retrieveBuffers();

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

    bool m_isRecording;

    BufferCallback m_bufferCallback;
    BlockingReaderWriterQueue<std::vector<float>> m_bufferQueue;
};

};  // namespace reformant

#endif  // REFORMANT_AUDIO_AUDIOINPUT_H