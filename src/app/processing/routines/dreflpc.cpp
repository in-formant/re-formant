#include "routines.h"

void dreflpc(const std::vector<double>& c, std::vector<double>& a, int n) {
    a[0] = 1.;
    a[1] = c[0];

    for (int i = 2; i <= n; ++i) {
        a[i] = c[i - 1];
        for (int j = 1; j <= i / 2; ++j) {
            const int k = i - j;
            const double ta1 = a[j] + c[i - 1] * a[k];
            a[k] += a[j] * c[i - 1];
            a[j] = ta1;
        }
    }
}