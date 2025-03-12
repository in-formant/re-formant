#include "ftrack/dsp.hpp"

#include <algorithm>

namespace ftrack {
namespace dsp {
int_type SRH_numframes(const int_type len, const int_type Fs) {
    const int_type stop = std::round(100.0 / 1000.0 * Fs);
    const int_type shift = std::round(10.0 / 1000.0 * Fs);
    const int_type Nframes = std::floor((len - stop) / shift) + 1;

    return Nframes + 10; // zero-pad with 5 frames on each end.
}

void SRH_EstimatePitch(const float_type* sig, const int_type sig_len,
                       const int_type Fs, const int_type F0min, const int_type F0max,
                       int_type* f0, float_type* SRHVal) {
    int_type start = 0;
    int_type stop = std::round(100.0 / 1000.0 * Fs);

    const int_type shift = std::round(10.0 / 1000.0 * Fs);

    const int_type Nframes = std::floor((sig_len - stop) / shift) + 1;
    constexpr int_type Npad = 5;

    const int_type win_len = stop - start + 1;
    ArrayXf blackwin = blackman(win_len);
    ArrayXf seg(win_len);

    int_type index = 1;
    while (stop < sig_len) {
        seg = Eigen::Map<const ArrayXf>(sig + start, win_len);
        seg *= blackwin;
        seg -= seg.mean();

        ArrayXf spec = abs(rfft(seg, Fs));
        spec /= std::sqrt(spec.square().sum());

        // SRH spectral criterion
        ArrayXf SRHs = ArrayXf::Zero(F0max + 1);
        for (int_type freq = F0min; freq <= F0max; ++freq) {
            constexpr int_type Nharm = 5;
            for (int_type n = 1; n <= Nharm; ++n) {
                // 1 - 2 - 3 - 4 - 5
                SRHs(freq) += spec(n * freq);
            }
            for (int_type n = 1; n < Nharm; ++n) {
                // 1.5 - 2.5 - 3.5 - 4.5
                SRHs(freq) -= spec(static_cast<int_type>(std::round((n + 0.5) * freq)));
            }
        }

        int_type F0frame;
        float_type maxVal = SRHs.maxCoeff(&F0frame);

        f0[Npad + index] = F0frame;
        SRHVal[Npad + index] = maxVal;

        start += shift;
        stop += shift;
        index++;
    }

    for (int i = 0; i < Npad; ++i) {
        f0[i] = f0[Npad + Nframes + i] = 0;
        SRHVal[i] = SRHVal[Npad + Nframes + i] = 0;
    }
}

void SRH_PitchTracking(const float_type* sig, const int_type sig_len,
                       const int_type Fs, int_type F0min, int_type F0max,
                       int_type* f0, int_type* VUVDecisions, float_type* SRHVal) {
    const int_type LPCorder = static_cast<int_type>(std::round(3.0 / 4.0 * Fs / 1000.0));
    constexpr int_type Niter = 2;

    ArrayXf res(sig_len);
    lpcresidual(sig, sig_len, std::round(25.0 / 1000.0 * Fs),
                std::round(5.0 / 1000.0 * Fs),
                LPCorder, res.data());
    const int_type Nframes = SRH_numframes(sig_len, Fs);

    std::vector<float_type> posiF0;

    // Estimate the pitch track in 2 iterations
    for (int_type iter = 1; iter <= Niter; ++iter) {
        SRH_EstimatePitch(res.data(), sig_len, Fs, F0min, F0max, f0, SRHVal);

        posiF0.clear();
        for (int_type i = 0; i < Nframes; ++i) {
            if (SRHVal[i] > 0.1) {
                posiF0.push_back(f0[i]);
            }
        }

        if (!posiF0.empty()) {
            float_type F0meanEst = median(posiF0);

            F0min = std::round(0.5 * F0meanEst);
            F0max = std::round(2.0 * F0meanEst);
        }
    }

    // Voiced-Unvoiced decisions are derived from the SRH value.
    Eigen::Map<ArrayXf> srh(SRHVal, Nframes);
    float_type std_srh = std::sqrt((srh - srh.mean()).square().sum() / (Nframes - 1));
    float_type threshold = (std_srh > 0.05) ? 0.085 : 0.07;

    for (int_type i = 0; i < Nframes; ++i) {
        VUVDecisions[i] = (SRHVal[i] > threshold);
    }
}
}
}
