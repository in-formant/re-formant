#include "routines.h"

void rwindow(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
             double preEmphasis) {
    if (preEmphasis != 0.) {
        for (int i = 0; i < n; ++i) {
            out[i] = in[ioff + i + 1] - preEmphasis * in[ioff + i];
        }
    } else {
        for (int i = 0; i < n; ++i) {
            out[i] = in[ioff + i];
        }
    }
}
