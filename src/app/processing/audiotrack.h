#ifndef REFORMANT_PROCESSING_AUDIOTRACK_H
#define REFORMANT_PROCESSING_AUDIOTRACK_H

#include <speex_resampler.h>

#include <mutex>
#include <vector>

namespace reformant {

class AudioTrack {
   public:
    AudioTrack();
    virtual ~AudioTrack();

    void append(const std::vector<float>& chunk, double sampleRate);
    void reset();

    void setSampleRate(double sampleRate);

    double sampleRate() const;
    double duration() const;
    int sampleCount() const;

    std::vector<float> data(int offset = 0, int length = -1);

    std::timed_mutex& mutex();

   private:
    void resampleTrack(double fsIn, double fsOut);

    double m_sampleRate;

    std::timed_mutex m_mutex;
    std::vector<float> m_track;

    SpeexResamplerState* m_resampler;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_AUDIOTRACK_H
