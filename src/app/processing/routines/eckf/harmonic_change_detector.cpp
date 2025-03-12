#include "ECKF.h"

#include <cmath>
#include <cstdint>

namespace {
uint64_t next_pow2(uint64_t x);

void blackman(int len, std::vector<double>& w);

void truncated_fbins(int nfft, int nbins_below50, double fs, std::vector<double>& fbins);

void poisson_window(int M, double alpha, std::vector<double>& w);
}

ECKF::HarmReturn ECKF::harmonic_change_detector(const std::vector<double>& xprev,
                                                const std::vector<double>& xcur,
                                                double fs,
                                                int nPeaks,
                                                int nSemitones) {
    // One-off calculation.
    static int minf, nfft, nbins_below50;
    static std::vector<double> fbins, win, exp_win;
    if (fbins.empty()) {
        minf = xcur.size();
        nfft = next_pow2(4 * (minf + 1));
        //Blackman
        blackman(minf, win);
        //considering minimum possible frequency to be 50Hz, we ignore all
        //bins that are below 50Hz. Number of bins below 50Hz = 50/(fs/2*nfft)
        nbins_below50 = static_cast<int>(std::round(50 / (fs / 2 * nfft)));
        truncated_fbins(nfft, nbins_below50, fs, fbins);
        poisson_window(fbins.size(), 5, exp_win);
    }

    return {};
}

namespace {
uint64_t next_pow2(uint64_t x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

void blackman(int len, std::vector<double>& w) {
    constexpr double a0 = 7938 / 18608.0;
    constexpr double a1 = 9240 / 18608.0;
    constexpr double a2 = 1430 / 18608.0;
    w.resize(len);
    for (int i = 0; i < len; ++i) {
        w[i] = a0 - a1 * cos((2 * M_PI * i) / (len - 1))
               + a2 * cos((4 * M_PI * i) / (len - 1));
    }
}

void truncated_fbins(int nfft, int nbins_below50, double fs, std::vector<double>& fbins) {
    // Only keep after nfft/2 + nbins_below50 (inclusive)

    const int offset = nfft / 2 + nbins_below50;

    const int n = nfft;
    const int size = n - offset + 1;
    fbins.resize(size);

    const double min = -fs / 2;
    const double max = fs / 2;

    for (int i = offset; i <= n - 2; ++i) {
        fbins[i - offset] = min + i * (max - min) / static_cast<double>(n - 1);
    }
    fbins[n - 1 - offset] = max;
}

void poisson_window(int M, double alpha, std::vector<double>& w) {
    // w = exp(-0.5*alpha*(0:M-1)./(M-1))';
    w.resize(M);
    for (int i = 0; i < M; ++i) {
        w[i] = exp(-0.5 * alpha * i / static_cast<double>(M - 1));
    }
}
}