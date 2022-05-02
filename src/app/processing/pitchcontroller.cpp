#include "pitchcontroller.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>

#include "../state.h"
#include "util/util.h"

using namespace reformant;

namespace {
void subtractReferenceMean(std::vector<float>& s);
std::vector<float> downsampleSignal(const std::vector<float>& s, int off,
                                    int len, double Fs, double Fds,
                                    Resampler& resampler);
void calculateDownsampledNCCF(const std::vector<float>& dss, int dsn, int dsK1,
                              int dsK2, std::vector<double>& dsNCCF);
void calculateOriginalNCCF(
    const std::vector<float>& s, int off, double Fs, double Fds, int n, int K,
    const std::vector<std::pair<double, double>>& dsPeaks,
    std::vector<double>& nccf);
std::vector<std::pair<double, double>> findPeaksWithThreshold(
    const std::vector<double>& nccf, double cand_tr, int n_cands,
    bool paraInterp);
}  // namespace

PitchController::PitchController(AppState& appState)
    : appState(appState),
      m_dsResampler(4),
      m_lastTime(0),
      m_minSilenceRunLength(0),
      m_minVoicingRunLength(0),
      m_pitchBuffer(std::max(m_minSilenceRunLength, m_minVoicingRunLength) + 1,
                    PitchPoint{0, -1}) {
    F0min = 50;
    F0max = 600;
    cand_tr = 0.3;
    lag_wt = 0.3;
    freq_wt = 0.02;
    vtran_c = 0.005;
    vtr_a_c = 0.5;
    vtr_s_c = 0.5;
    vo_bias = -0.2;
    doubl_c = 0.35;
    a_fact = 10000;
    n_cands = 20;
}

void PitchController::forceClear(bool lock) {
    if (lock) m_mutex.lock();

    m_times.clear();
    m_pitches.clear();
    std::fill(m_pitchBuffer.begin(), m_pitchBuffer.end(), PitchPoint{0, -1});

    m_dsResampler.reset();
    m_dsResampler.skipZeros();

    if (lock) m_mutex.unlock();
}

void PitchController::updateIfNeeded() {
    using namespace std::chrono_literals;

    // Just return if we couldn't lock, this isn't important because it's
    // ran periodically, and it avoids a potential deadlock.
    std::unique_lock trackLock(appState.audioTrack.mutex(), 50ms);
    if (!trackLock.owns_lock()) return;

    std::lock_guard lockGuard(m_mutex);

    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    // Track length in seconds.
    const double trackDuration = trackSamples / Fs;

    // Correlation window size. (secs)
    const double w = 0.0075;

    // Total window size.
    const double windowLength = w + 1 / F0min;

    if (!m_times.empty() && trackDuration < m_lastTime) {
        return;
    }

    const int n = (int)std::round(w * Fs);
    const int K = (int)std::round(Fs / F0min);
    const int wl = n + K;

    const double Fds = std::round(Fs / std::round(Fs / (4 * F0max)));
    const int dsn = (int)std::round(w * Fds);
    const int dsK1 = (int)std::round(Fds / F0max);
    const int dsK2 = (int)std::round(Fds / F0min);
    const int dswl = dsn + dsK2;

    const double beta = lag_wt / (Fs / F0min);

    const int J = std::round(0.03 * Fs);

    const int trackIndex0 = (int)std::round(m_lastTime * Fs);

    auto s =
        appState.audioTrack.data(trackIndex0, trackSamples - trackIndex0 - 1);

    subtractReferenceMean(s);

    int is = 0;

    std::vector<double> dsNCCF(dsK2 + 1);
    std::vector<double> nccf(K + 1);

    while (is + wl < s.size()) {
        auto dss = downsampleSignal(s, is, wl, Fs, Fds, m_dsResampler);
        if (dss.size() < dswl) {
            dss.resize(dswl, 0);
        }
        calculateDownsampledNCCF(dss, dsn, dsK1, dsK2, dsNCCF);
        auto dsPeaks = findPeaksWithThreshold(dsNCCF, cand_tr, n_cands, false);

        const double time =
            (trackIndex0 + is - m_dsResampler.inputLatency()) / Fs;
        double pitch = -1;

        if (!dsPeaks.empty()) {
            calculateOriginalNCCF(s, is, Fs, Fds, n, K, dsPeaks, nccf);
            auto peaks = findPeaksWithThreshold(nccf, cand_tr, n_cands, false);

            // Find the candidate with the lowest cost.
            int minLag = 0;
            double minCost = std::numeric_limits<double>::max();
            double maxVal = std::numeric_limits<double>::min();

            for (const auto& [k, y] : peaks) {
                const double localCost = 1 - y * (1 - beta * k);
                if (localCost < minCost) {
                    minLag = k;
                    minCost = localCost;
                }
                if (y > maxVal) {
                    maxVal = y;
                }
            }

            if (vo_bias + maxVal >= minCost) {
                const double Linterp =
                    util::parabolicInterpolation(nccf, minLag).first;
                pitch = Fs / Linterp;
            }
        }

        is += wl;

        m_pitchBuffer.back() = PitchPoint{time, pitch};

        // Check voicing run length.
        auto& bufferFront = m_pitchBuffer.front();
        if (bufferFront.pitch < 0) {
            bool isTooShort = false;
            double replacePitch;
            for (int k = 1; k < m_minSilenceRunLength; ++k) {
                if (m_pitchBuffer[k].pitch > 0) {
                    isTooShort = true;
                    replacePitch = m_pitchBuffer[k].pitch;
                    break;
                }
            }
            if (isTooShort) {
                bufferFront.pitch = replacePitch;
            }
        }
        if (bufferFront.pitch > 0) {
            bool isLongEnough = true;
            for (int k = 1; k < m_minVoicingRunLength; ++k) {
                if (m_pitchBuffer[k].pitch < 0) {
                    isLongEnough = false;
                    break;
                }
            }
            if (isLongEnough) {
                m_times.push_back(bufferFront.time);
                m_pitches.push_back(bufferFront.pitch);
            }
        }

        std::rotate(m_pitchBuffer.begin(), m_pitchBuffer.begin() + 1,
                    m_pitchBuffer.end());
    }

    m_lastTime = trackDuration;
}

namespace {
void subtractReferenceMean(std::vector<float>& s) {
    double mu = 0;
    for (int j = 0; j < s.size(); ++j) mu += s[j];
    mu /= s.size();
    for (int j = 0; j < s.size(); ++j) s[j] -= mu;
}

std::vector<float> downsampleSignal(const std::vector<float>& s, const int off,
                                    const int len, const double Fs,
                                    const double Fds, Resampler& resampler) {
    resampler.setRate(Fs, Fds);
    resampler.reset();
    resampler.skipZeros();
    return resampler.process(s, off, len);
}

void calculateDownsampledNCCF(const std::vector<float>& dss, const int dsn,
                              const int dsK1, const int dsK2,
                              std::vector<double>& dsNCCF) {
    double dse0 = 0;
    for (int l = 0; l < dsn; ++l) {
        dse0 += dss[l] * dss[l];
    }

    for (int k = dsK1; k <= dsK2; ++k) {
        double p(0), q(0);

        for (int j = 0; j < dsn; ++j) {
            p += dss[j] * dss[j + k];
        }

        for (int l = k; l < k + dsn; ++l) {
            q += dss[l] * dss[l];
        }

        q = sqrt(dse0 * q);

        dsNCCF[k] = p / q;
    }
}

void calculateOriginalNCCF(
    const std::vector<float>& s, const int off, const double Fs,
    const double Fds, const int n, const int K,
    const std::vector<std::pair<double, double>>& dsPeaks,
    std::vector<double>& nccf) {
    std::vector<int> lagsToCalculate;

    for (const auto& [dsk, y] : dsPeaks) {
        const int k = (int)std::round((Fs * dsk) / Fds);

        if (k >= 0 && k <= K) lagsToCalculate.push_back(k);

        for (int l = 1; l <= 3; ++l) {
            if (k - l >= 0 && k - l <= K) lagsToCalculate.push_back(k - l);
            if (k + l >= 0 && k + l <= K) lagsToCalculate.push_back(k + l);
        }
    }

    std::sort(lagsToCalculate.begin(), lagsToCalculate.end());
    auto last = std::unique(lagsToCalculate.begin(), lagsToCalculate.end());
    lagsToCalculate.erase(last, lagsToCalculate.end());

    std::fill(nccf.begin(), nccf.end(), 0);

    double e0;
    {
        int j = 0;
        double v = 0;
        for (int l = j; l < j + n; ++l) {
            v += s[off + l] * s[off + l];
        }
        e0 = v;
    }

    for (const int k : lagsToCalculate) {
        double p(0), q(0);

        for (int j = 0; j < n; ++j) {
            p += s[off + j] * s[off + j + k];
        }

        for (int l = k; l < k + n; ++l) {
            q += s[off + l] * s[off + l];
        }

        q = sqrt(e0 * q);

        nccf[k] = p / q;
    }
}

std::vector<std::pair<double, double>> findPeaksWithThreshold(
    const std::vector<double>& nccf, const double cand_tr, const int n_cands,
    const bool paraInterp) {
    double max = std::numeric_limits<double>::lowest();
    for (int i = 0; i < nccf.size(); ++i) {
        if (nccf[i] > max) {
            max = nccf[i];
        }
    }

    const double threshold = cand_tr * max;

    auto allPeaks = util::findPeaks(nccf);

    std::vector<std::pair<double, double>> peaks;

    for (const int k : allPeaks) {
        if (k >= 0 && k < nccf.size() && nccf[k] > threshold) {
            peaks.push_back(util::parabolicInterpolation(nccf, k));
        }
    }

    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (peaks.size() > n_cands - 1) {
        peaks.erase(std::next(peaks.begin(), n_cands - 1), peaks.end());
    }

    return peaks;
}
}  // namespace

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

            result.times.push_back(time);
            result.pitches.push_back(pitch);
        }
    }

    return result;
}
