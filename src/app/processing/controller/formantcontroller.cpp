#include "formantcontroller.h"

#include <iostream>

#include "../../state.h"
#include "../routines/routines.h"

using namespace reformant;

namespace {
void subtractReferenceMean(std::vector<float>& s);
std::vector<float> downsampleSignal(const std::vector<float>& s, int off, int len,
                                    double Fs, double Fds, Resampler& resampler);
}  // namespace

FormantController::FormantController(AppState& appState)
    : appState(appState),
      m_dsResampler(4),
      m_lastTime(0),
      m_lastSampleRate(-1),
      m_tracking(4, -10.0) {}

void FormantController::forceClear(bool lock) {
    if (lock) m_mutex.lock();

    m_times.clear();
    m_frequencies.clear();

    m_dsResampler.reset();
    m_dsResampler.skipZeros();

    if (lock) m_mutex.unlock();
}

void FormantController::updateIfNeeded() {
    using namespace std::chrono_literals;

    // Just return if we couldn't lock, this isn't important because it's
    // ran periodically, and it avoids a potential deadlock.
    std::unique_lock trackLock(appState.audioTrack.mutex(), 50ms);
    if (!trackLock.owns_lock()) return;

    std::lock_guard lockGuard(m_mutex);

    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    if (Fs != m_lastSampleRate) {
        m_lastTime = std::round((m_lastTime / m_lastSampleRate) * Fs);
        m_lastSampleRate = Fs;
    }

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    if (!m_times.empty() && trackSamples < m_lastTime) {
        return;
    }

    constexpr double windowDuration = 15.0 / 1000.0;     // Window size of each LPC frame
    constexpr double frameIntervalTime = 10.0 / 1000.0;  // Time between each LPC frame
    const int frameInterval = (int)std::round(frameIntervalTime * Fs);

    constexpr double analysisDuration = 500.0 / 1000.0;  // Approx time of each analysis
    constexpr double analysisGap = 100.0 / 1000.0;  // Approx gap between each analysis

    // Exact sample counts.
    const int analysisSamplesEx = (int)std::round(analysisDuration * Fs);
    const int analysisGapSamplesEx = (int)std::round(analysisGap * Fs);

    // Sample counts rounded to an integer multiple of frameInterval
    const int analysisSamples = (analysisSamplesEx / frameInterval) * frameInterval;
    const int analysisGapSamples = (analysisGapSamplesEx / frameInterval) * frameInterval;

    // Find largest negative track offset.
    int trackOffset = 0;
    while (trackOffset < analysisGapSamples && m_lastTime - trackOffset >= 0) {
        trackOffset += frameInterval;
    }
    if (trackOffset > 0) trackOffset -= frameInterval;

    const int trackIndex = m_lastTime - trackOffset;

    // Find largest analysis length.
    int analysisLength = 0;
    while (analysisLength < analysisSamples &&
           trackIndex + analysisLength <= trackSamples) {
        analysisLength += frameInterval;
    }
    analysisLength -= frameInterval;

    if (analysisLength < frameInterval) {
        m_lastTime = (trackSamples / frameInterval) * frameInterval;
        return;
    }

    m_lastTime += analysisGapSamples;

    const auto os = appState.audioTrack.data(trackIndex, analysisLength);

    constexpr double Fds = 11000.0;
    const double trackIndexDs = (trackIndex / Fs) * Fds;

    m_dsResampler.setRate(Fs, Fds);
    m_dsResampler.reset();
    m_dsResampler.skipZeros();
    const auto ds = m_dsResampler.process(os);

    std::vector<double> s(ds.begin(), ds.end());
    for (auto& x : s) x *= std::numeric_limits<int16_t>::max();

    const auto ps = lpc_poles(s, Fds, windowDuration, frameIntervalTime, 12, 0.97,
                              LPC_AUTOC, WINDOW_COS4);

    const auto track = m_tracking.track(ps);
    // track.form : (nForm, ps.length)

    if (track.form.cols() < 1) return;

    // Replace time range with new tracked range.
    const double firstTime = trackIndex / Fs;

    auto it = m_times.begin();
    auto it2 = m_frequencies.begin();
    while (it != m_times.end()) {
        if (*it >= firstTime) {
            it = m_times.erase(it);
            it2 = m_frequencies.erase(it2);
        } else {
            ++it;
            ++it2;
        }
    }

    for (int i = 0; i < track.form.rows(); ++i) {
        for (int j = 0; j < track.form.cols(); ++j) {
            const double time = (trackIndexDs + track.form(i, j).offset) / Fds;
            m_times.push_back(time);
            m_frequencies.push_back(track.form(i, j).freq);
        }
    }
}

FormantResults FormantController::getFormantsForRange(double timeMin, double timeMax,
                                                      double tpp) {
    std::lock_guard lockGuard(m_mutex);

    FormantResults result;

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    for (int i = 0; i < m_times.size(); ++i) {
        const double time = m_times[i];
        if (time >= timeMin && time <= timeMax) {
            const double frequency = m_frequencies[i];

            result.times.push_back(time);
            result.frequencies.push_back(frequency);
        }
    }

    return result;
}