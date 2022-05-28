#include <iostream>

#include "routines.h"

void autoc(int windowSize, const std::vector<double>& s, int p, std::vector<double>& r,
           double* e) {
    double sum0 = 0.;
    for (int i = 0; i < windowSize; ++i) {
        sum0 += s[i] * s[i];
    }
    r[0] = 1.; /* r[0] will always =1. */

    if (sum0 == 0.) {
        /* No energy: fake low-energy white noise. */
        *e = 1.; /* Arbitrarily assign 1 to rms. */
        /* Now fake autocorrelation of white noise. */
        for (int i = 1; i <= p; ++i) {
            r[i] = 0;
        }
        return;
    }

    for (int i = 1; i <= p; ++i) {
        double sum = 0.;
        for (int j = 0; j < windowSize - i; ++j) {
            sum += s[j] * s[i + j];
        }
        r[i] = sum / sum0;
    }
    if (sum0 < 0.) {
        std::cerr << "autoc(): sum0 = " << sum0 << std::endl;
    }
    *e = sqrt(sum0 / windowSize);
}