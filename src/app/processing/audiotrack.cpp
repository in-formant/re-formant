#include "audiotrack.h"

#include <algorithm>
#include <mutex>

using namespace reformant;

AudioTrack::AudioTrack() : m_sampleRate(0), m_doDenoising(false) {
    // m_trackPtr.reset(std::make_shared<std::vector<float>>());
}

void AudioTrack::append(const std::vector<float>& chunkInput, const double fsIn) {
    const double fsOut = m_sampleRate;

    m_resamplerTo48kHz.setRate(fsIn, 48000);
    m_resamplerToTrack.setRate(48000, fsOut);

    auto chunk48kHz = m_resamplerTo48kHz.process(chunkInput);

    // If denoising is enabled, process it now.
    // (The denoiser only takes 48kHz audio)
    if (m_doDenoising) {
        chunk48kHz = m_denoiser.process(chunk48kHz);
    }

    const auto chunk = m_resamplerToTrack.process(chunk48kHz);

    // m_trackPtr.copy_update([&chunk](std::vector<float>* copy) {
    //     copy->insert(copy->end(), chunk.begin(), chunk.end());
    // });
    m_mutex.lock();
    m_track.insert(m_track.end(), chunk.begin(), chunk.end());
    m_mutex.unlock();
}

void AudioTrack::reset() {
    // m_trackPtr.copy_update([](std::vector<float>* copy) { copy->clear(); });
    m_mutex.lock();
    m_track.clear();
    m_mutex.unlock();
}

void AudioTrack::setSampleRate(double sampleRate) {
    const double oldSR = m_sampleRate;
    m_sampleRate = sampleRate;

    if (oldSR > 0 && sampleRate != oldSR) {
        resampleTrack(oldSR, sampleRate);
    }
}

void AudioTrack::setDenoising(bool denoising) { m_doDenoising = denoising; }

double AudioTrack::sampleRate() const { return m_sampleRate; }

double AudioTrack::duration() const { return sampleCount() / m_sampleRate; }

int AudioTrack::sampleCount() const {
    return m_track.size();
    // std::shared_ptr<const std::vector<float>> local_copy = m_trackPtr.read();
    // return local_copy->size();
}

bool AudioTrack::isDenoising() const { return m_doDenoising; }

std::vector<float> AudioTrack::data(const int offset, int length) const {
    // std::shared_ptr<const std::vector<float>> local_copy = m_trackPtr.read();

    // if (offset < 0 || offset >= local_copy->size()) {
    //     return {};
    // }

    // if (length < 0) {
    //     length = local_copy->size() - offset;
    // }

    // std::vector<float> copy(length);
    // std::copy_n(local_copy->begin() + offset, length, copy.begin());

    if (offset < 0 || offset >= m_track.size()) {
        return {};
    }

    if (length < 0) {
        length = m_track.size() - offset;
    }
    if (offset + length - 1 >= m_track.size()) {
        // std::cout << "requested too many samples" << std::endl;
        length = m_track.size() - offset;
    }

    std::vector<float> copy(length);
    std::copy_n(m_track.begin() + offset, length, copy.begin());

    return copy;
}

std::shared_mutex& AudioTrack::mutex() { return m_mutex; }

void AudioTrack::resampleTrack(const double fsIn, const double fsOut) {
    m_resamplerToTrack.setRate(48000, fsOut);

    // std::shared_ptr<const std::vector<float>> local_copy = m_trackPtr.read();

    // if (!local_copy->empty()) {
    //     Resampler resampler(fsIn, fsOut);
    //     std::vector<float> resed = resampler.process(*local_copy);
    //     resed.insert(resed.begin(), m_resamplerToTrack.outputLatency(), 0);

    //     m_trackPtr.copy_update([&resed](std::vector<float>* copy) { *copy = resed; });
    // }

    m_mutex.lock();

    if (!m_track.empty()) {
        Resampler resampler(fsIn, fsOut);
        m_track = resampler.process(m_track);
        m_track.insert(m_track.begin(), m_resamplerToTrack.outputLatency(), 0);
    }

    m_mutex.unlock();
}
