#include "audiotrack.h"

#include <cmath>
#include <iostream>

using namespace reformant;

AudioTrack::AudioTrack() : m_sampleRate(0) {
    int error;
    m_resampler = speex_resampler_init(1, 48000, 48000, 10, &error);
    if (error != 0) {
        std::cerr << "Speex new error: " << speex_resampler_strerror(error)
                  << std::endl;
        exit(EXIT_FAILURE);
    }
}

AudioTrack::~AudioTrack() { speex_resampler_destroy(m_resampler); }

void AudioTrack::append(const std::vector<float>& chunk, const double fsIn) {
    const double fsOut = m_sampleRate;

    int error;

    // Set the ratio immediately, it will only change because of a device
    // change so there's no need to transition smoothly.
    error = speex_resampler_set_rate(m_resampler, fsIn, fsOut);
    if (error != 0) {
        std::cerr << "Speex set ratio error: "
                  << speex_resampler_strerror(error) << std::endl;
        return;
    }

    uint32_t ilen = chunk.size();
    uint32_t olen;
    speex_get_expected_output_frame_count(m_resampler, ilen, &olen);

    std::vector<float> resampledChunk(olen);

    error = speex_resampler_process_float(m_resampler, 0, chunk.data(), &ilen,
                                          resampledChunk.data(), &olen);
    if (error != 0) {
        std::cerr << "Speex process error: " << speex_resampler_strerror(error)
                  << std::endl;
        return;
    }

    m_track.insert(m_track.end(), resampledChunk.begin(),
                   resampledChunk.begin() + olen);
}

void AudioTrack::reset() { m_track.clear(); }

void AudioTrack::setSampleRate(double sampleRate) {
    const double oldSR = m_sampleRate;
    m_sampleRate = sampleRate;

    if (oldSR > 0 && sampleRate != oldSR) {
        resampleTrack(oldSR, sampleRate);
    }
}

double AudioTrack::sampleRate() const { return m_sampleRate; }

double AudioTrack::duration() const { return m_track.size() / m_sampleRate; }

int AudioTrack::sampleCount() const { return m_track.size(); }

std::vector<float> AudioTrack::data(const int offset, int length) {
    if (offset < 0 || offset >= m_track.size()) {
        return {};
    }

    if (length < 0) {
        length = m_track.size() - offset;
    }

    std::vector<float> copy(length);
    std::copy(m_track.begin() + offset, m_track.begin() + offset + length,
              copy.begin());

    return copy;
}

std::timed_mutex& AudioTrack::mutex() { return m_mutex; }

void AudioTrack::resampleTrack(const double fsIn, const double fsOut) {
    int error;

    error = speex_resampler_reset_mem(m_resampler);
    if (error != 0) {
        std::cerr << "Speex reset error: " << speex_resampler_strerror(error)
                  << std::endl;
        return;
    }

    error = speex_resampler_skip_zeros(m_resampler);
    if (error != 0) {
        std::cerr << "Speex skip zeros error: "
                  << speex_resampler_strerror(error) << std::endl;
        return;
    }

    error = speex_resampler_set_rate(m_resampler, fsIn, fsOut);
    if (error != 0) {
        std::cerr << "Speex set ratio error: "
                  << speex_resampler_strerror(error) << std::endl;
        return;
    }

    uint32_t ilen = m_track.size();
    uint32_t olen;
    speex_get_expected_output_frame_count(m_resampler, ilen, &olen);

    std::vector<float> out(olen);

    error = speex_resampler_process_float(m_resampler, 0, m_track.data(), &ilen,
                                          out.data(), &olen);
    if (error != 0) {
        std::cerr << "Speex process error: " << speex_resampler_strerror(error)
                  << std::endl;
        return;
    }

    out.resize(olen);

    m_track = std::move(out);
}