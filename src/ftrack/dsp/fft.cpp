#include "ftrack/dsp.hpp"
#include <fftw3.h>

namespace ftrack {
namespace dsp {
namespace detail {
fftwf_complex* fftw_cast(complex_type* x) {
    return reinterpret_cast<fftwf_complex*>(x);
}

struct plan_wrapper final {
    int_type N_;
    bool fwd_;
    fftwf_plan plan_;

    plan_wrapper(const int_type N, const bool fwd, complex_type* in, complex_type* out) {
        N_ = N;
        fwd_ = fwd;
        plan_ = fftwf_plan_dft_1d(N, fftw_cast(in), fftw_cast(out),
                                  fwd ? FFTW_FORWARD : FFTW_BACKWARD, FFTW_PATIENT);
    }

    plan_wrapper(const int_type N, const bool fwd, float_type* in, complex_type* out) {
        assert(fwd == true);
        N_ = N;
        fwd_ = fwd;
        plan_ = fftwf_plan_dft_r2c_1d(N, in, fftw_cast(out), FFTW_PATIENT);
    }

    plan_wrapper(const int_type N, const bool fwd, complex_type* in, float_type* out) {
        assert(fwd == false);
        N_ = N;
        fwd_ = fwd;
        plan_ = fftwf_plan_dft_c2r_1d(N, fftw_cast(in), out, FFTW_PATIENT);
    }

    ~plan_wrapper() {
        fftwf_destroy_plan(plan_);
    }
};

std::vector<plan_wrapper> plans;

template <typename In, typename Out>
fftwf_plan get_or_make_plan(const int_type N, const bool fwd, In* in,
                            Out* out) {
    for (auto& p : plans) {
        if (p.N_ == N && p.fwd_ == fwd) {
            return p.plan_;
        }
    }
    return plans.emplace_back(N, fwd, in, out).plan_;
}

template <typename ArrayType>
ArrayType zero_pad_or_truncate(const ArrayType& x, const int_type L) {
    ArrayType y(L);
    if (x.size() < L) {
        y(seq(0, x.size() - 1)) = x;
        y(seq(x.size(), last)) = 0;
    } else if (x.size() > L) {
        y = x.leftCols(L);
    } else {
        y = x;
    }
    return y;
}
} // namespace detail

ArrayXcf fft(const ArrayXcf& xi, const int_type N) {
    ArrayXcf x = detail::zero_pad_or_truncate(xi, N);
    ArrayXcf y(x.size());
    fftwf_execute_dft(detail::get_or_make_plan(N, true, x.data(), y.data()),
                      detail::fftw_cast(x.data()), detail::fftw_cast(y.data()));
    return y;
}

ArrayXcf ifft(const ArrayXcf& xi, const int_type N) {
    ArrayXcf x = detail::zero_pad_or_truncate(xi, N);
    ArrayXcf y(x.size());
    fftwf_execute_dft(detail::get_or_make_plan(N, false, x.data(), y.data()),
                      detail::fftw_cast(x.data()), detail::fftw_cast(y.data()));
    return y;
}

ArrayXcf rfft(const ArrayXf& xi, const int_type N) {
    ArrayXf x = detail::zero_pad_or_truncate(xi, N);
    ArrayXcf y(x.size() / 2 + 1);
    fftwf_execute_dft_r2c(detail::get_or_make_plan(N, true, x.data(), y.data()),
                          x.data(), detail::fftw_cast(y.data()));
    return y;
}

ArrayXf irfft(const ArrayXcf& xi, const int_type N) {
    ArrayXcf x = detail::zero_pad_or_truncate(xi, N);
    ArrayXf y(2 * (x.size() / 2 + 1));
    fftwf_execute_dft_c2r(detail::get_or_make_plan(N, false, x.data(), y.data()),
                          detail::fftw_cast(x.data()), y.data());
    return y;
}
} // namespace dsp
} // namespace ftrack