#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
void lpcresidual(const float_type* sig, const int_type sig_len,
                 const int_type L, const int_type shift, const int_type order,
                 float_type* res) {
    int_type start = 0;
    int_type stop = start + L;

    ArrayXf hannwin = hann(L);
    ArrayXf segment(L);

    ArrayXf A(order + 1);
    float_type e;

    ArrayXf inv(L);

    Eigen::Map<ArrayXf> Res(res, sig_len);

    while (stop < sig_len) {
        segment = Eigen::Map<const ArrayXf>(sig + start, L);
        segment *= hannwin;

        lpc(segment.data(), L, order, A.data(), &e);

        filter(segment.data(), L, A.data(), order + 1, inv.data());
        inv *= sqrt(segment.square().sum() / inv.square().sum());

        Res(seq(start, stop)) += inv;

        start += shift;
        stop += shift;
    }

    // normalize
    Res /= Res.abs().maxCoeff();
}
}
}