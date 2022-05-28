
#include "routines.h"

void dlwrtrn(const std::vector<double>& a, int n, std::vector<double>& x,
             const std::vector<double>& y) {
    x[0] = y[0] / a[0];
    for (int i = 1; i < n; ++i) {
        double sm = y[i];
        for (int j = 0; j < i + 1; ++i) {
            sm -= a[i * n + j] * x[j];
        }
        x[i + 1] = sm / a[i * (n + 1) + 1];
    }
}