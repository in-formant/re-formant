#include "audiotrack.h"

#include <cmath>
#include <iostream>

using namespace reformant;

AudioTrack::AudioTrack() : m_sampleRate(0) {}

void AudioTrack::append(const std::vector<float>& chunk, const double fsIn) {
    const double fsOut = m_sampleRate;

    m_resamplerTo48kHz.setRate(fsIn, 48000);
    m_resamplerToTrack.setRate(48000, fsOut);

    auto chunk48kHz = m_resamplerTo48kHz.process(chunk);

    // If denoising is enabled, process it now.
    // (The denoiser only takes 48kHz audio)
    if (m_doDenoising) {
        chunk48kHz = m_denoiser.process(chunk48kHz);
    }

    const auto trackChunk = m_resamplerToTrack.process(chunk48kHz);

    m_track.insert(m_track.end(), trackChunk.begin(), trackChunk.end());
}

void AudioTrack::reset() { m_track.clear(); }

void AudioTrack::setSampleRate(double sampleRate) {
    const double oldSR = m_sampleRate;
    m_sampleRate = sampleRate;

    if (oldSR > 0 && sampleRate != oldSR) {
        resampleTrack(oldSR, sampleRate);
    }
}

void AudioTrack::setDenoising(bool denoising) { m_doDenoising = denoising; }

double AudioTrack::sampleRate() const { return m_sampleRate; }

double AudioTrack::duration() const { return m_track.size() / m_sampleRate; }

int AudioTrack::sampleCount() const { return m_track.size(); }

bool AudioTrack::isDenoising() const { return m_doDenoising; }

std::vector<float> AudioTrack::data(const int offset, int length) {
    if (offset < 0 || offset >= m_track.size()) {
        return {};
    }

    if (length < 0) {
        length = m_track.size() - offset;
    }

    std::vector<float> copy(length);
    std::copy(m_track.begin() + offset, m_track.begin() + offset + length, copy.begin());

    return copy;
}

std::timed_mutex& AudioTrack::mutex() { return m_mutex; }

void AudioTrack::resampleTrack(const double fsIn, const double fsOut) {
    m_resamplerToTrack.setRate(48000, fsOut);

    if (!m_track.empty()) {
        Resampler resampler(fsIn, fsOut);
        m_track = resampler.process(m_track);
        m_track.insert(m_track.begin(), m_resamplerToTrack.outputLatency(), 0);
    }
}