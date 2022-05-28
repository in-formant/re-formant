#include "routines.h"

void durbin(const std::vector<double>& r, std::vector<double>& k, std::vector<double>& a,
            int p, double* ex) {
    static std::vector<double> b(MAXORDER);

    double e = r[0];
    k[0] = -r[1] / e;
    a[0] = k[0];

    e *= (1. - k[0] * k[0]);

    for (int i = 1; i < p; ++i) {
        double s = 0.;
        for (int j = 0; j < i; ++j) {
            s -= a[j] * r[i - j];
        }
        k[i] = (s - r[i + 1]) / e;
        a[i] = k[i];
        for (int j = 0; j <= i; ++j) {
            b[j] = a[j];
        }
        for (int j = 0; j < i; ++j) {
            a[j] += k[i] * b[i - j - 1];
        }
        e *= (1. - k[i] * k[i]);
    }
    *ex = e;
}