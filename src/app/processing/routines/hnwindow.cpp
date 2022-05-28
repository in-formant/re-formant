#include "routines.h"

void hnwindow(const std::vector<int>& in, int ioff, std::vector<double>& out, int n,
              double preEmphasis) {
    static std::vector<double> wind;

    if (wind.size() != n) {
        wind.resize(n);
        const double arg = M_PI * 2.0 / n;
        for (int i = 0; i < n; ++i) {
            wind[i] = .5 - .5 * cos((i + .5) * arg);
        }
    }

    if (preEmphasis != 0.) {
        for (int i = 0; i < n; ++i) {
            out[i] =
                wind[i] * (double(in[ioff + i + 1]) - preEmphasis * double(in[ioff + i]));
        }
    } else {
        for (int i = 0; i < n; ++i) {
            out[i] = wind[i] * double(in[ioff + i]);
        }
    }
}
