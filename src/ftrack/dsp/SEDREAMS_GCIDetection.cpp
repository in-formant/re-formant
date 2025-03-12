#include "ftrack/dsp.hpp"
#include "ftrack/AlignedAlloc.hpp"

#include <deque>

namespace ftrack {
namespace dsp {
std::vector<int_type> SEDREAMS_GCIDetection(const float_type* sig, int_type sig_len,
                                            int_type Fs, float_type F0mean) {
    ArrayXf res(sig_len);
    lpcresidual(sig, sig_len, std::round(25.0 / 1000.0 * Fs),
                std::round(5.0 / 1000.0 * Fs), std::round(Fs / 1000) + 2, res.data());

    Eigen::Map<const ArrayXf> wave(sig, sig_len);
    ArrayXf MeanBasedSignal = ArrayXf::Zero(sig_len);
    int_type T0mean = static_cast<int_type>(std::round(Fs / F0mean));

    int_type halfL = static_cast<int_type>(std::round((1.7 * T0mean) / 2));
    ArrayXf blackwin = blackman(2 * halfL + 1);

    std::deque<int_type> maxis;
    std::vector<int_type, AlignedAllocator<int_type> > minis;
    ArrayXi PosMeanBasedSignal = ArrayXi::Zero(sig_len);

    int_type m, tmp;

    ArrayXf vec(2 * halfL + 1);

    std::vector<int_type> posis;
    // m += 2^4
    for (m = halfL; m < sig_len - halfL + 1; m += 1 << 4) {
        vec = wave(seq(m - halfL, m + halfL));
        vec *= blackwin;
        MeanBasedSignal(m) = vec.mean();
        PosMeanBasedSignal(m) = 1;
        posis.push_back(m);
    }

    for (tmp = 1; tmp < static_cast<int_type>(posis.size()) - 1; ++tmp) {
        if (MeanBasedSignal(posis[tmp]) > MeanBasedSignal(posis[tmp - 1])
            && MeanBasedSignal(posis[tmp]) > MeanBasedSignal(posis[tmp + 1])) {
            maxis.push_back(posis[tmp]);
        }

        if (MeanBasedSignal(posis[tmp]) < MeanBasedSignal(posis[tmp - 1])
            && MeanBasedSignal(posis[tmp]) < MeanBasedSignal(posis[tmp + 1])) {
            minis.push_back(posis[tmp]);
        }
    }

    for (int_type StepExp = 3; StepExp >= 0; --StepExp) {
        int_type Step = 1 << StepExp; // 2^StepExp

        for (tmp = 0; tmp < maxis.size(); ++tmp) {
            m = maxis[tmp] - Step;
            if (!PosMeanBasedSignal(m)) {
                vec = wave(seq(m - halfL, m + halfL));
                vec *= blackwin;
                MeanBasedSignal(m) = vec.mean();
                PosMeanBasedSignal(m) = 1;
            }

            m = maxis[tmp] + Step;
            if (!PosMeanBasedSignal(m)) {
                vec = wave(seq(m - halfL, m + halfL));
                vec *= blackwin;
                MeanBasedSignal(m) = vec.mean();
                PosMeanBasedSignal(m) = 1;
            }

            const float_type v1 = MeanBasedSignal(maxis[tmp] - Step);
            const float_type v2 = MeanBasedSignal(maxis[tmp]);
            const float_type v3 = MeanBasedSignal(maxis[tmp] + Step);

            int_type offset;
            if (v1 > v2) {
                if (v1 > v3) {
                    //v1
                    offset = -Step;
                } else {
                    //v3
                    offset = +Step;
                }
            } else {
                if (v2 > v3) {
                    //v2
                    offset = 0;
                } else {
                    //v3
                    offset = +Step;
                }
            }
            maxis[tmp] += offset;
        }

        for (tmp = 0; tmp < minis.size(); ++tmp) {
            m = minis[tmp] - Step;
            if (!PosMeanBasedSignal(m)) {
                vec = wave(seq(m - halfL, m + halfL));
                vec *= blackwin;
                MeanBasedSignal(m) = vec.mean();
                PosMeanBasedSignal(m) = 1;
            }

            m = minis[tmp] + Step;
            if (!PosMeanBasedSignal(m)) {
                vec = wave(seq(m - halfL, m + halfL));
                vec *= blackwin;
                MeanBasedSignal(m) = vec.mean();
                PosMeanBasedSignal(m) = 1;
            }

            const float_type v1 = MeanBasedSignal(minis[tmp] - Step);
            const float_type v2 = MeanBasedSignal(minis[tmp]);
            const float_type v3 = MeanBasedSignal(minis[tmp] + Step);

            int_type offset;
            if (v1 < v2) {
                if (v1 < v3) {
                    //v1
                    offset = -Step;
                } else {
                    //v3
                    offset = +Step;
                }
            } else {
                if (v2 < v3) {
                    //v2
                    offset = 0;
                } else {
                    //v3
                    offset = +Step;
                }
            }
            minis[tmp] += offset;
        }
    }

    MeanBasedSignal /= MeanBasedSignal.abs().maxCoeff();

    if (maxis.front() < minis.front()) {
        maxis.pop_front();
    }

    if (minis.back() > maxis.back()) {
        minis.pop_back();
    }

    // Determine the median position of GCIs within the cycle
    res /= res.abs().maxCoeff();

    Eigen::Map<ArrayXi> Minis(minis.data(), minis.size());

    std::vector<float_type> RelPosis;
    for (int_type posi = 0; posi < sig_len; ++posi) {
        if (res(posi) > 0.4) {
            int_type minpos;
            (Minis - posi).abs().minCoeff(&minpos);
            const int_type interv = maxis[minpos] - minis[minpos];

            RelPosis.push_back((posi - minis[minpos]) / static_cast<float_type>(interv));
        }
    }

    float_type ratioGCI = median(RelPosis);

    // Detect GCIs from the residual wave using the presence intervals derived from the mean-based wave
    std::vector<int_type> gci;
    float_type alpha;
    int_type start, stop;

    for (int_type k = 0; k < minis.size(); ++k) {
        const int_type interv = maxis[k] - minis[k];
        alpha = ratioGCI - 0.25;
        start = minis[k] + static_cast<int_type>(std::round(alpha * interv));
        alpha = ratioGCI + 0.35;
        stop = minis[k] + static_cast<int_type>(std::round(alpha * interv));

        if (start < 0) {
            start = 0;
        }

        if (stop >= sig_len) {
            stop = sig_len - 1;
        }

        vec = res(seq(start, stop));
        int_type posi;
        vec.maxCoeff(&posi);
        gci.push_back(start + posi - 1);
    }

    return gci;
}
}
}