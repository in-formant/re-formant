#include "routines.h"

bool formant(int lpcOrder, double sFreq, std::vector<double>& lpca, int* nForm,
             std::vector<double>& freq, std::vector<double>& band, bool init) {
    std::vector<double> rr(lpcOrder + 1);
    std::vector<double> ri(lpcOrder + 1);

    if (init) { /* set up starting points for the root search near unit circle */
        const double x = M_PI / (lpcOrder + 1);
        for (int i = 0; i <= lpcOrder; ++i) {
            const double flo = lpcOrder - i;
            rr[i] = 2. * cos((flo + .5) * x);
            ri[i] = 2. * sin((flo + .5) * x);
        }
    }

    /* find the roots of the LPC polynomial */
    if (!lbpoly(lpca, lpcOrder, rr, ri)) { /* was there a problem in the root finder? */
        *nForm = 0;
        return false;
    }

    const double pi2t = M_PI * 2. / sFreq;

    /* convert the z-plane locations to frequencies and bandwidths */
    for (int ii = 0; ii < lpcOrder; ++ii) {
        if (rr[ii] != 0. || ri[ii] != 0.) {
            const double theta = atan2(ri[ii], rr[ii]);
            const double freqii = fabs(theta / pi2t);
            double bandii =
                .5 * sFreq * log((rr[ii] * rr[ii]) + (ri[ii] * ri[ii])) / M_PI;
            if (bandii < 0.) {
                bandii = -bandii;
            }
            freq.push_back(freqii);
            band.push_back(bandii);

            /* complex pole? */
            if ((rr[ii] == rr[ii + 1]) && (ri[ii] == -ri[ii + 1]) && (ri[ii] != 0.)) {
                ii++; /* if so, don't duplicate */
            }
        }
    }

    /* now order the complex poles by frequency. always place the (uninteresting)
       real poles at the end of the arrays */
    const double theta = sFreq / 2.; /* temp. hold the folding frequency */
    const int fc = freq.size();

    bool iscomp1, iscomp2, swit;
    double flo;

    /* bubble sort */
    for (int i = 0; i < fc - 1; ++i) {
        for (int ii = 0; ii < fc - 1 - i; ++ii) {
            /* Force the real poles to the end of the list */
            iscomp1 = (freq[ii] > 1.) && (freq[ii] < theta);
            iscomp2 = (freq[ii + 1] > 1.) && (freq[ii + 1] < theta);
            swit = (freq[ii] > freq[ii + 1]) && iscomp2;
            if (swit || (iscomp2 && !iscomp1)) {
                std::swap(freq[ii], freq[ii + 1]);
                std::swap(band[ii], band[ii + 1]);
            }
        }
    }

    /* now count the complex poles as formant candidates */
    int ii = 0;
    for (int i = 0; i < fc; ++i) {
        if (freq[i] > 1. && freq[i] < theta - 1.) ii++;
    }
    *nForm = ii;
    freq.resize(ii);
    band.resize(ii);

    return true;
}