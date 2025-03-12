#include "ftrack/dsp.hpp"

namespace ftrack {
namespace dsp {
namespace detail {
/*
all the pointer arguments are shifted by one to the left,
which means the 0-th element is OUT OF BOUNDS!!!!
    x: n
    a: m
    b1: n
    b2: n
    aa: m+1
*/
float_type vecBurg(const float_type* x, const int_type n, const int_type m,
                   float_type* a, float_type* b1, float_type* b2, float_type* aa,
                   bool* valid) {
    for (int_type i = 1; i <= m; ++i) {
        a[i] = 0.0;
    }
    if (n <= 2) {
        a[1] = -1.0;
        return (n == 2 ? 0.5 * (x[1] * x[1] + x[2] * x[2]) : x[1] * x[1]);
    }

    float_type p = 0.0;
    for (int_type i = 1; i <= n; ++i) {
        p += x[i] * x[i];
    }

    if (p == 0.0) {
        *valid = false;
        return 0.0;
    }

    b1[1] = x[1];
    b2[n - 1] = x[n];
    for (int_type j = 2; j <= n - 1; j++)
        b1[j] = b2[j - 1] = x[j];

    long double xms = p / n;
    for (int_type i = 1; i <= m; i++) {
        // (7)

        long double num = 0.0, denum = 0.0;
        for (int_type j = 1; j <= n - i; j++) {
            num += b1[j] * b2[j];
            denum += b1[j] * b1[j] + b2[j] * b2[j];
        }

        if (denum <= 0.0) {
            *valid = false;
            return 0.0; // warning ill-conditioned
        }
        a[i] = 2.0 * static_cast<float_type>(num / denum);

        // (10)

        xms *= 1.0 - a[i] * a[i];

        // (5)

        for (int_type j = 1; j <= i - 1; j++)
            a[j] = aa[j] - a[i] * aa[i - j];

        if (i < m) {
            // (8) Watch out: i -> i+1

            for (int_type j = 1; j <= i; j++)
                aa[j] = a[j];
            for (int_type j = 1; j <= n - i - 1; j++) {
                b1[j] -= aa[i] * b2[j];
                b2[j] = b2[j + 1] - aa[i] * b1[j + 1];
            }
        }
    }
    *valid = true;
    return static_cast<float_type>(xms);
}
}

void lpc(const float_type* x, const int_type n, const int_type m,
         float_type* lpca, float_type* energy) {
    /*
    x: n
    a: m
    b1: n
    b2: n
    aa: m+1
    */
    ArrayXf a(m + 1);
    ArrayXf b1(n + 1);
    ArrayXf b2(n + 1);
    ArrayXf aa(m + 2);

    bool valid;
    float_type gain = detail::vecBurg(&x[-1], n, m, a.data(),
                                      b1.data(), b2.data(), aa.data(), &valid);

    if (valid && gain > 0.0) {
        *energy = gain * n;
        lpca[0] = 1;
        for (int_type i = 1; i <= m; ++i) {
            lpca[i] = -a(i);
        }
    } else {
        *energy = 0.0;
    }
}
}
}