#include "routines.h"

bool lpc(int lpcOrd, double lpcStabl, int wsize, const std::vector<int>& data,
         int dataOff, std::vector<double>& lpca, double* rms, double preEmphasis,
         WindowType windowType) {
    static std::vector<double> dwind;
    static std::vector<double> rho(MAXORDER + 1);
    static std::vector<double> k(MAXORDER + 1);
    static std::vector<double> a(MAXORDER);

    if (wsize <= 0 || lpcOrd > MAXORDER) return false;
    if (dwind.size() != wsize) {
        dwind.resize(wsize);
    }

    w_window(data, dataOff, dwind, wsize, preEmphasis, windowType);

    double en, er;
    autoc(wsize, dwind, lpcOrd, rho, &en);

    if (lpcStabl > 1.) { /* add a little to the diagonal for stability */
        const double ffact = 1. / (1. + exp((-lpcStabl / 20.) * log(10.)));
        for (int i = 1; i <= lpcOrd; ++i) {
            rho[i] = ffact * rho[i];
        }
    }
    durbin(rho, k, a, lpcOrd, &er);

    *rms = en;
    return true;
}