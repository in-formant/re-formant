#include "pitchcontroller.h"

#include <cmath>
#include <complex>

#include "../../state.h"
#include "../util/util.h"

using namespace reformant;

PitchController::PitchController(AppState& appState)
    : appState(appState),
      m_lastTimeRapt(0),
      m_lastTimeCrepe(0),
      m_lastSampleRate(-1),
      m_rapt(appState),
      m_crepe(appState) {}

void PitchController::forceClear(const bool lock) {
    if (lock) m_mutex.lock();

    m_lastTimeRapt = m_lastTimeCrepe = 0;
    m_lastSampleRate = -1;

    m_rapt.reset();
    m_crepe.reset();

    m_times.clear();
    m_pitches.clear();
    m_saliences.clear();

    if (lock) m_mutex.unlock();
}

void PitchController::updateIfNeeded() {
    std::shared_lock trackLock(appState.audioTrack.mutex());

    std::lock_guard lockGuard(m_mutex);

    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    if (Fs != m_lastSampleRate) {
        m_lastTimeRapt = std::round((m_lastTimeRapt / m_lastSampleRate) * Fs);
        m_lastTimeCrepe = std::round((m_lastTimeCrepe / m_lastSampleRate) * Fs);
        m_lastSampleRate = Fs;
    }

    // -- RAPT
    //m_rapt.process(m_lastTimeRapt, m_times, m_pitches, m_saliences);
    m_crepe.process(m_lastTimeCrepe, m_times, m_pitches, m_saliences);
}

PitchResults PitchController::getPitchesForRange(double timeMin, double timeMax,
                                                 double timePerPixel) {
    std::lock_guard lockGuard(m_mutex);

    PitchResults result;

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    for (int i = 0; i < m_times.size(); ++i) {
        const double time = m_times[i];
        if (time >= timeMin && time <= timeMax) {
            const double pitch = m_pitches[i];
            const double salience = m_saliences[i];

            if (!std::isnan(pitch) && !std::isnan(salience) && salience > 0.15) {
                result.times.push_back(time);
                result.pitches.push_back(pitch);
                result.saliences.push_back(salience);
            }
        }
    }

    return result;
}

static inline double voicing(double pitch) { return pitch < 0 ? 0 : 1; }

double PitchController::getInterpolatedVoicing(double x) const {
    const int n = m_times.size();

    int indexLeft, indexRight;

    if (x < m_times[0]) {
        indexLeft = indexRight = 0;
    } else if (x >= m_times.back()) {
        indexLeft = indexRight = n - 1;
    } else {
        for (int i = 0; i < n - 1; ++i) {
            if (m_times[i] <= x && x < m_times[i + 1]) {
                indexLeft = i;
                indexRight = i + 1;
            }
        }
    }

    if (indexLeft == indexRight) {
        return (m_pitches[indexLeft] < 0) ? 0 : 1;
    }

    const double x0 = m_times[indexLeft];
    const double x1 = m_times[indexRight];

    const double p0 = voicing(m_pitches[indexLeft]);
    const double p1 = voicing(m_pitches[indexRight]);

    constexpr double t0 = 0;
    constexpr double t1 = 1;
    const double t = (x - x0) / (x1 - x0);

    // Cubic Hermite interpolation.

    const double t2 = t * t;
    const double t3 = t * t2;

    const double h00 = 2 * t3 - 3 * t2 + 1;
    const double h10 = t3 - 2 * t2 + t;
    const double h01 = -2 * t3 + 3 * t2;
    const double h11 = t3 - t2;

    const double dp1 = p1 - p0;
    const double dx1 = x1 - x0;

    const double dp0 = (indexLeft > 0) ? p0 - voicing(m_pitches[indexLeft - 1]) : dp1;
    const double dx0 = (indexLeft > 0) ? x0 - m_times[indexLeft - 1] : dx1;

    const double dp2 =
        (indexRight < n - 1) ? voicing(m_pitches[indexRight + 1]) - p1 : dp1;
    const double dx2 = (indexRight < n - 1) ? m_times[indexRight + 1] - x1 : dx1;

    const double df0 = dp0 / dx0;
    const double df1 = dp1 / dx1;
    const double df2 = dp2 / dx2;

    const double m0 = .5 * (df1 + df0);
    const double m1 = .5 * (df2 + df1);

    const double p = h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;

    return p;
}