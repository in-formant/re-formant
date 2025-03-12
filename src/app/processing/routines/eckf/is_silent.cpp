#include "ECKF.h"

#include <cmath>

namespace {
double geomean(const std::vector<float>& data) {
    double m = 1.0;
    long long ex = 0;
    double invN = 1.0 / data.size();

    for (double x : data) {
        int i;
        double f1 = std::frexp(x, &i);
        m *= f1;
        ex += i;
    }

    return std::pow(std::numeric_limits<double>::radix, ex * invN) * std::pow(m, invN);
}

double mean(const std::vector<float>& x) {
    double sum = 0;
    for (int i = 0; i < x.size(); ++i) {
        sum += x[i];
    }
    return sum / static_cast<double>(x.size());
}
}

bool ECKF::is_silent(const std::vector<double>& x) {
    constexpr double sf_threshold = 0.45;

    // method for clean signal - calculate signal energy and see
    // if it is below a certain threshold
    double energy = 0;
    for (int i = 0; i < x.size(); i++) {
        energy += x[i] * x[i];
    }
    energy = 20 * log10(energy);

    // method for noisy signals - find PSD of signal and its spectral flatness
    // the assumption is that silent frames have noise only
    // and therefore a flat power spectrum
    pwelch(x);
    const double spectralFlatness = geomean(psdw) / mean(psdw);

    cur_spf = spectralFlatness;

    return spectralFlatness > sf_threshold || energy < -50;
}

void ECKF::pwelch(const std::vector<double>& x) {
    std::ranges::copy(x, psdw.begin());
    for (int i = 0; i < x.size(); ++i) {
        psdw[i] *= w[i];
    }
    fftwf_execute(pwelch_plan);
    for (int i = 0; i < x.size(); ++i) {
        psdw[i] /= static_cast<double>(x.size());
    }
}