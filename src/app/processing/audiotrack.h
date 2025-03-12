#ifndef REFORMANT_PROCESSING_AUDIOTRACK_H
#define REFORMANT_PROCESSING_AUDIOTRACK_H

// #include <rcu_ptr.hpp>
#include <shared_mutex>
#include <vector>

#include "denoiser.h"
#include "resampler.h"

namespace reformant {

class AudioTrack {
   public:
    AudioTrack();

    void append(const std::vector<float>& chunk, double sampleRate);

    void reset();

    void setSampleRate(double sampleRate);

    void setDenoising(bool denoising);

    double sampleRate() const;

    double duration() const;

    int sampleCount() const;

    bool isDenoising() const;

    std::vector<float> data(int offset = 0, int length = -1) const;

    std::shared_mutex& mutex();

   private:
    void resampleTrack(double fsIn, double fsOut);

    double m_sampleRate;

    // rcu_ptr<std::vector<float>> m_trackPtr;

    mutable std::shared_mutex m_mutex;
    std::vector<float> m_track;

    Resampler m_resamplerTo48kHz;
    Resampler m_resamplerToTrack;

    bool m_doDenoising;
    Denoiser m_denoiser;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_AUDIOTRACK_H
