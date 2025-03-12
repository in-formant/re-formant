#include "waveformcontroller.h"

#include <cfloat>
#include <cmath>
#include <iostream>

#include "../../memusage.h"
#include "../../state.h"

using namespace reformant;

WaveformController::WaveformController(AppState& appState) : appState(appState),
    m_lastSampleRate(-1), m_waveResults{}, m_needWaveUpdate(false), m_waveTimeMin(0),
    m_waveTimeMax(0),
    m_waveTimePerPixel(0) {
}

WaveformController::~WaveformController() = default;

void WaveformController::forceClear(bool lock) {
    if (lock) m_mutex.lock();

    resetWaveformResults();
    m_needWaveUpdate = false;

    if (lock) m_mutex.unlock();
}

void WaveformController::updateIfNeeded() {
    using namespace std::chrono_literals;

    // Just return if we couldn't lock, this isn't important because it's
    // ran periodically, and it avoids a potential deadlock.
    std::unique_lock trackLock(appState.audioTrack.mutex(), 50ms);
    if (!trackLock.owns_lock()) return;

    std::lock_guard lockGuard(m_mutex);

    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    if (Fs != m_lastSampleRate) {
        m_lastSampleRate = Fs;
        m_needWaveUpdate = true;
    }

    if (m_needWaveUpdate) {
        updateWaveformResults();
        m_needWaveUpdate = false;
    }
}

const WaveformResults& WaveformController::getWaveformForRange(
    double timeMin, double timeMax, double timePerPixel) {
    std::lock_guard lockGuard(m_mutex);

    const double sampleRate = appState.audioTrack.sampleRate();

    // Extend by one pixel on each side.
    m_needWaveUpdate = true;
    m_waveTimeMin = timeMin - 10 * timePerPixel;
    m_waveTimeMax = timeMax + 10 * timePerPixel;
    m_waveTimePerPixel = timePerPixel;

    return m_waveResults;
}

void WaveformController::updateWaveformResults() {
    auto& wave = m_waveResults;

    const double sampleRate = appState.audioTrack.sampleRate();
    const int trackSampleCount = appState.audioTrack.sampleCount();

    const double timeMin = m_waveTimeMin;
    const double timeMax = m_waveTimeMax;
    const double timePerPixel = m_waveTimePerPixel;

    if (trackSampleCount == 0 || timePerPixel == 0) {
        resetWaveformResults();
        return;
    }

    const int startIndex = std::clamp(static_cast<int>(timeMin * sampleRate), 0,
                                      trackSampleCount - 1);
    const int stopIndex = std::clamp(static_cast<int>(timeMax * sampleRate), 0,
                                     trackSampleCount - 1);
    const int rangeSampleCount = stopIndex - startIndex + 1;

    const double actualTimeMin = startIndex / sampleRate;
    const double actualTimeMax = stopIndex / sampleRate;

    if (stopIndex < startIndex) {
        resetWaveformResults();
        return;
    }

    constexpr double ratioThresholdSamples = 0.2;
    constexpr double ratioThresholdMinMax = 0.025;
    constexpr double ratioThresholdRMS = 0.02;
    const double approxPixels = (m_waveTimeMax - m_waveTimeMin) / timePerPixel;
    const double approxSamples = (m_waveTimeMax - m_waveTimeMin) * sampleRate;
    const double zoomRatio = approxPixels / approxSamples;

    if (zoomRatio >= ratioThresholdSamples) {
        wave.timeMin = actualTimeMin;
        wave.timeMax = actualTimeMax;
        wave.timeScale = 1 / sampleRate;
        wave.type = WaveformDataType_Samples;
        wave.samples.resize(rangeSampleCount);
        const auto window = appState.audioTrack.data(startIndex, rangeSampleCount);
        std::ranges::copy(window, wave.samples.begin());
        return;
    }

    const int windowSize = static_cast<int>(std::round(timePerPixel * sampleRate));

    wave.times.clear();
    wave.mins.clear();
    wave.maxs.clear();
    wave.rms1.clear();
    wave.rms2.clear();

    const int roundedStartIndex = (startIndex / windowSize) * windowSize;

    int chunkStartIndex = roundedStartIndex;
    while (chunkStartIndex < stopIndex && chunkStartIndex + windowSize <
           trackSampleCount) {
        float min = std::numeric_limits<double>::max();
        float max = std::numeric_limits<double>::lowest();
        double sumOfSquares = 0;
        const auto window = appState.audioTrack.data(chunkStartIndex, windowSize);
        for (int i = 0; i < windowSize; ++i) {
            min = std::min(window[i], min);
            max = std::max(window[i], max);
            sumOfSquares += window[i] * window[i];
        }

        const double rms = sqrt(sumOfSquares / windowSize);

        wave.times.push_back(
            (chunkStartIndex + (windowSize + 1) / 2.0) /
            sampleRate);
        wave.mins.push_back(min);
        wave.maxs.push_back(max);
        wave.rms1.push_back(rms);
        wave.rms2.push_back(-rms);

        chunkStartIndex += windowSize;
    }

    wave.minmaxShaded = (zoomRatio >= ratioThresholdMinMax);

    // Only show RMS calculation if it is zoomed out enough.
    if (zoomRatio >= ratioThresholdRMS) {
        wave.type = WaveformDataType_MinMax;
    } else {
        wave.type = WaveformDataType_MinMaxRMS;
    }
}

void WaveformController::resetWaveformResults() {
    m_waveResults.timeMin = 0;
    m_waveResults.timeMax = 0;
    m_waveResults.timeScale = 1;
    m_waveResults.type = WaveformDataType_Samples;
    m_waveResults.times.clear();
    m_waveResults.samples.clear();
    m_waveResults.minmaxShaded = false;
    m_waveResults.mins.clear();
    m_waveResults.maxs.clear();
    m_waveResults.rms1.clear();
    m_waveResults.rms2.clear();
}