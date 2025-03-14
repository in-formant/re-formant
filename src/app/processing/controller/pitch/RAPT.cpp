#include "RAPT.h"
#include "../../util/util.h"

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

namespace reformant {
RAPT::RAPT(AppState& appState) : appState(appState),
                                 m_dsResampler(4) {
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

void RAPT::reset() {
    if (m_dsResampler.isValid()) {
        m_dsResampler.reset();
        m_dsResampler.skipZeros();
    }
}

void RAPT::process(int& lastTime, std::vector<double>& times,
                   std::vector<double>& pitches,
                   std::vector<double>& saliences) {
    // Track sample rate.
    const double Fs = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    if (!times.empty() && trackSamples < lastTime) { return; }

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

    const int trackIndex0 = lastTime;

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

                times.push_back(time);
                pitches.push_back(pitch);
                saliences.push_back(1.0);
            }

        }

        is += wl;
    }

    lastTime += is;
}
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

    std::ranges::sort(peaks, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    if (peaks.size() > n_cands - 1) {
        peaks.erase(std::next(peaks.begin(), n_cands - 1), peaks.end());
    }

    return peaks;
}
} // namespace