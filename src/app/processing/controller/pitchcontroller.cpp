#include "pitchcontroller.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>

#include "../../state.h"
#include "../util/util.h"

using namespace reformant;

namespace {
void subtractReferenceMean(std::vector<float>& s);

std::vector<float> downsampleSignal(const std::vector<float>& s, int off, int len,
                                    double Fs, double Fds, Resampler& resampler);

void calculateDownsampledNCCF(const std::vector<float>& dss, int dsn, int dsK1, int dsK2,
                              std::vector<double>& dsNCCF);

void calculateOriginalNCCF(const std::vector<float>& s, int off, double Fs, double Fds,
                           int n, int K,
                           const std::vector<std::pair<double, double> >& dsPeaks,
                           std::vector<double>& nccf);

std::vector<std::pair<double, double> > findPeaksWithThreshold(
    const std::vector<double>& nccf, double cand_tr, int n_cands, bool paraInterp);
} // namespace

PitchController::PitchController(AppState& appState)
    : appState(appState),
      m_dsResampler(4),
      m_lastTime(0),
      m_lastSampleRate(-1),
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

    m_dsResampler.reset();
    m_dsResampler.skipZeros();

    m_lastTime = 0;
    m_lastSampleRate = -1;

    m_times.clear();
    m_pitches.clear();

    m_minSilenceRunLength = 0;
    m_minVoicingRunLength = 0;
    std::fill(m_pitchBuffer.begin(), m_pitchBuffer.end(), PitchPoint{0, -1});

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

    if (Fs != m_lastSampleRate) {
        m_lastTime = std::round((m_lastTime / m_lastSampleRate) * Fs);
        m_lastSampleRate = Fs;
    }

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    if (!m_times.empty() && trackSamples < m_lastTime) {
        return;
    }

    // Correlation window size. (secs)
    constexpr double w = 0.0075;

    const int n = static_cast<int>(std::round(w * Fs));
    const int K = static_cast<int>(std::round(Fs / F0min));
    const int wl = n + K;

    const double Fds = std::round(Fs / std::round(Fs / (4 * F0max)));
    const int dsn = static_cast<int>(std::round(w * Fds));
    const int dsK1 = static_cast<int>(std::round(Fds / F0max));
    const int dsK2 = static_cast<int>(std::round(Fds / F0min));
    const int dswl = dsn + dsK2;

    const double beta = lag_wt / (Fs / F0min);

    const int J = static_cast<int>(std::round(0.03 * Fs));

    const int trackIndex0 = m_lastTime;

    auto s = appState.audioTrack.data(trackIndex0, trackSamples - trackIndex0 - 1);

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

        const double time = (trackIndex0 + is - m_dsResampler.inputLatency()) / Fs;
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
                const double Linterp = util::parabolicInterpolation(nccf, minLag).first;
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

    m_lastTime += is;
}

namespace {
void subtractReferenceMean(std::vector<float>& s) {
    double mu = 0;
    for (int j = 0; j < s.size(); ++j) mu += s[j];
    mu /= s.size();
    for (int j = 0; j < s.size(); ++j) s[j] -= mu;
}

std::vector<float> downsampleSignal(const std::vector<float>& s, const int off,
                                    const int len, const double Fs, const double Fds,
                                    Resampler& resampler) {
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

void calculateOriginalNCCF(const std::vector<float>& s, const int off, const double Fs,
                           const double Fds, const int n, const int K,
                           const std::vector<std::pair<double, double> >& dsPeaks,
                           std::vector<double>& nccf) {
    std::vector<int> lagsToCalculate;

    for (const auto& [dsk, y] : dsPeaks) {
        const int k = static_cast<int>(std::round((Fs * dsk) / Fds));

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

    double e0; {
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

std::vector<std::pair<double, double> > findPeaksWithThreshold(
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

    std::vector<std::pair<double, double> > peaks;

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
} // namespace

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

static inline double voicing(double pitch) { return pitch < 0 ? 0 : 1; }

double PitchController::getInterpolatedVoicing(double x) {
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