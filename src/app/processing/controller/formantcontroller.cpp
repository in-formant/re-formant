#include "formantcontroller.h"

#include "../../state.h"
#include "../routines/routines.h"

using namespace reformant;

FormantController::FormantController(AppState& appState)
    : appState(appState), m_dsResampler(4), m_lastTime(0), m_tracking(4, -10.0) {}

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

    constexpr double frameIntervalTime = 50.0 / 1000.0;
    const int frameInterval = std::round(frameIntervalTime * Fs);
    const int trackIndex0 = std::max(0, m_lastTime - 10 * frameInterval);

    if (!m_times.empty() && trackSamples < trackIndex0) {
        return;
    }

    const auto os = appState.audioTrack.data(trackIndex0, trackSamples - trackIndex0 - 1);

    constexpr double Fds = 10000.0;

    m_dsResampler.setRate(Fs, Fds);
    m_dsResampler.reset();
    m_dsResampler.skipZeros();
    const auto ds = m_dsResampler.process(os);

    // Expects signed 16-bit integer range as double.
    std::vector<double> s(ds.size());
    for (int i = 0; i < ds.size(); ++i) {
        s[i] = ds[i] * std::numeric_limits<int16_t>::max();
    }

    const auto ps =
        lpc_poles(s, Fds, 15.0 / 1000.0, frameIntervalTime, 12, 50.0, LPC_BSA, WINDOW_COS4);

    const auto track = m_tracking.track(ps);
    // track.form : (nForm, ps.length)

    // Replace time range with new tracked range.
    const int firstOffset = track.form(0, 0).offset;
    const int lastOffset = track.form(0, track.form.cols() - 1).offset;

}

FormantResults FormantController::getFormantsForRange(double timeMin, double timeMax,
                                                      double tpp) {
    return {};
}