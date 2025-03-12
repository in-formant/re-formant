#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
namespace detail {
void filter(const float_type* x, int_type len_x,
            const float_type* b, const float_type* a, int_type len_b,
            float_type* y, float_type* Z) {
    // TDF-II implementation from scipy

    auto ptr_x = x;
    auto ptr_y = y;

    for (int_type k = 0; k < len_x; ++k) {
        auto ptr_b = b;
        auto ptr_a = a;
        auto xn = x;
        auto yn = y;
        if (len_b > 1) {
            auto ptr_Z = Z;
            *yn = *ptr_Z + *ptr_b * *xn; /* Calculate first delay (output) */
            ptr_b++;
            ptr_a++;
            /* Fill in middle delays */
            for (int_type n = 0; n < len_b - 2; ++n) {
                *ptr_Z =
                    ptr_Z[1] + *xn * (*ptr_b) - *yn * (*ptr_a);
                ptr_b++;
                ptr_a++;
                ptr_Z++;
            }
            /* Calculate last delay */
            *ptr_Z = *xn * (*ptr_b) - *yn * (*ptr_a);
        } else {
            *yn = *xn * (*ptr_b);
        }

        ptr_y++;
        ptr_x++;
    }
}
}

void filter(const float_type* x, const int_type len_x,
            const float_type* b_, const int_type len_b,
            const float_type* a_, const int_type len_a,
            float_type* y, float_type* Z_) {
    // Zero-pad and normalize a and b.
    const int_type len_ab = std::max(len_a, len_b);
    ArrayXf b = ArrayXf::Zero(len_ab);
    ArrayXf a = ArrayXf::Zero(len_ab);
    b.leftCols(len_b) = Eigen::Map<const ArrayXf>(b_, len_b) / a_[0];
    a.leftCols(len_a) = Eigen::Map<const ArrayXf>(a_, len_a) / a_[0];

    if (Z_ != nullptr) {
        detail::filter(x, len_x, b.data(), a.data(), len_ab, y, Z_);
    } else {
        // Make a temporary zero-filled state vector.
        ArrayXf Z = ArrayXf::Zero(len_ab);
        detail::filter(x, len_x, b.data(), a.data(), len_ab, y, Z.data());
    }
}

void filter(const float_type* x, int_type len_x,
            const float_type* b, int_type len_b,
            float_type* y, float_type* Z_) {
    ArrayXf a = ArrayXf::Zero(len_b);
    a(0) = 1.0;

    if (Z_ != nullptr) {
        detail::filter(x, len_x, b, a.data(), len_b, y, Z_);
    } else {
        // Make a temporary zero-filled state vector.
        ArrayXf Z = ArrayXf::Zero(len_b);
        detail::filter(x, len_x, b, a.data(), len_b, y, Z.data());
    }
}
}
}