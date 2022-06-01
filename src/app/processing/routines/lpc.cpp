#include "routines.h"

bool lpc(int lpcOrd, double lpcStabl, int wsize, const std::vector<double>& data,
         int dataOff, std::vector<double>& lpca, double* rms, double preEmphasis,
         WindowType windowType) {
    static std::vector<double> dwind;
    // static std::vector<double> rho(MAXORDER + 1);
    // static std::vector<double> k(MAXORDER + 1);
    // static std::vector<double> a(MAXORDER);

    if (wsize <= 0 || lpcOrd > MAXORDER) return false;
    if (dwind.size() != wsize) {
        dwind.resize(wsize);
    }

    w_window(data, dataOff, dwind, wsize, preEmphasis, windowType);

    // double en, er;
    // autoc(wsize, dwind, lpcOrd, rho, &en);

    // if (lpcStabl > 1.) { /* add a little to the diagonal for stability */
    //     const double ffact = 1. / (1. + exp((-lpcStabl / 20.) * log(10.)));
    //     for (int i = 1; i <= lpcOrd; ++i) {
    //         rho[i] = ffact * rho[i];
    //     }
    // }
    // durbin(rho, k, a, lpcOrd, &er);

    // *rms = en;
    // return true;

    const int n = wsize;
    const int m = lpcOrd;

    std::vector<double> r(m + 2);
    std::vector<double> a(m + 2);
    std::vector<double> rc(m + 1);
    double gain;
    int i, j;

    j = m + 1;
    while (j--) {
        double d = 0.;
        for (i = j; i < n; ++i) d += dwind[i] * dwind[i - j];
        r[j + 1] = d;
    }
    if (r[1] == 0.) {
        return false;
    }

    a[1] = 1.0;
    a[2] = rc[1] = -r[2] / r[1];
    gain = r[1] + r[2] * rc[1];

    for (i = 2; i <= m; ++i) {
        double s = 0.;
        for (j = 1; j <= i; ++j) s += r[i - j + 2] * a[j];
        rc[i] = -s / gain;
        for (j = 2; j <= i / 2 + 1; ++j) {
            const double at = a[j] + rc[i] * a[i - j + 2];
            a[i - j + 2] += rc[i] * a[j];
            a[j] = at;
        }
        a[i + 1] = rc[i];
        gain += rc[i] * s;
        if (gain <= 0.) {
            return false;
        }
    }

    --i;

    *rms = gain;

    lpca.resize(i + 1);
    lpca[0] = 1.;
    for (j = 1; j <= i; ++j) lpca[j] = a[j + 1];
    return true;
}