#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
template <typename Float>
constexpr float_type v(Float x) {
    return float_type(x);
}

namespace detail {
template <float_type a0, float_type a1, float_type a2 = float_type(0),
          float_type a3 = float_type(0), float_type a4 = float_type(0)>
ArrayXf cos_win(const int_type N) {
    ArrayXf h(N);
    for (int_type i = 0; i < N; i++) {
        h(i) = a0 - a1 * std::cos((2 * M_PI * i) / (N - 1)) +
               a2 * std::cos((4 * M_PI * i) / (N - 1)) -
               a3 * std::cos((6 * M_PI * i) / (N - 1)) +
               a4 * std::cos((8 * M_PI * i) / (N - 1));
    }
    return h;
}
}  // namespace detail

ArrayXf hamming(const int_type len) { return detail::cos_win<v(0.54), v(0.46)>(len); }

ArrayXf hann(const int_type len) { return detail::cos_win<v(0.5), v(0.5)>(len); }

ArrayXf blackman(const int_type len) {
    return detail::cos_win<v(0.42), v(0.5), v(0.08)>(len);
}

ArrayXf blackmanharris(const int_type len) {
    return detail::cos_win<v(0.35875), v(0.48829), v(0.14128), v(0.01168)>(len);
}

ArrayXf flattopwin(const int_type len) {
    return detail::cos_win<v(0.21557895), v(0.41663158), v(0.277263158), v(0.083578947),
                           v(0.006947368)>(len);
}
}  // namespace dsp
}  // namespace ftrack