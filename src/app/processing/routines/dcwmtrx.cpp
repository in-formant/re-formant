#include "routines.h"

void dcwmtrx(const std::vector<double>& s, int ni, int nl, int np,
             std::vector<double>& phi, std::vector<double>& shi, double* ps,
             const std::vector<double>& w) {
    *ps = 0.;
    for (int i = ni; i < nl; ++i) {
        *ps += s[i] * s[i] * w[i - ni];
    }

    for (int i = 0; i < np; ++i) {
        shi[i] = 0.;
        for (int j = 0; j < nl - ni; ++j) {
            shi[i] += s[ni + j] * s[ni - i + j - 1] * w[j];
        }
    }

    for (int i = 0; i < np; ++i) {
        for (int j = 0; j <= i; ++j) {
            double sm = 0.;
            for (int k = 0; k < nl - ni; ++k) {
                sm += s[ni - i - 1 + k] * s[ni - j - 1 + k] * w[k];
            }
            phi[np * i + j] = sm;
            phi[np * j + i] = sm;
        }
    }
}