#include "routines.h"

bool w_covar(const std::vector<double>& xx, int xoff, int* m, int n, int istrt,
             std::vector<double>& y, double* alpha, double* r0, double preEmphasis,
             WindowType windowType)
{
    static int nold = 0;
    static int mold = 0;

    static std::vector<double> x;
    static std::vector<double> b;
    static std::vector<double> beta;
    static std::vector<double> grc;
    static std::vector<double> cc;

    if (n + 1 > nold) {
        x.resize(n + 1);
        nold = n + 1;
    }

    if (*m > mold) {
        const int mn = *m;

        b.resize((mn + 1) * (mn + 1) / 2);
        beta.resize(mn + 3);
        grc.resize(mn + 3);
        cc.resize(mn + 3);
        mold = mn;
    }

    w_window(xx, xoff, x, n, preEmphasis, windowType);

    const int ibeg = istrt - 1;
    const int ibeg1 = ibeg + 1;
    const int mp = *m + 1;
    const int ibegm1 = ibeg - 1;
    const int ibeg2 = ibeg + 2;
    const int ibegmp = ibeg + mp;
    const int msq = (*m + (*m) * (*m)) / 2;

    for (int i = 1; i <= msq; ++i) b[i] = 0.;

    *alpha = 0.;
    cc[1] = 0.;
    cc[2] = 0.;
    for (int np = mp; np <= n; ++np) {
        const int np1 = np + ibegm1;
        const int np0 = np + ibeg;
        *alpha += x[np0] * x[np0];
        cc[1] += x[np0] * x[np1];
        cc[2] += x[np1] * x[np1];
    }
    *r0 = *alpha;

    b[1] = 1.;
    beta[1] = cc[2];
    grc[1] = -cc[1] / cc[2];
    y[0] = 1.;
    y[1] = grc[1];
    *alpha += grc[1] * cc[1];

    if (*m <= 1) return false; /* need to correct indices?? */

    int mf = *m;
    for (int minc = 2; minc <= mf; ++minc) {
        for (int j = 1; j <= minc; ++j) {
            const int jp = minc + 2 - j;
            const int n1 = ibeg1 + mp - jp;
            const int n2 = ibeg1 + n - minc;
            const int n3 = ibeg2 + n - jp;
            cc[jp] = cc[jp - 1] + x[ibegmp - minc] * x[n1] - x[n2] * x[n3];
        }
        cc[1] = 0.;
        for (int np = mp; np <= n; ++np) {
            const int npb = np + ibeg;
            cc[1] += x[npb - minc] * x[npb];
        }
        const int msub = (minc * minc - minc) / 2;
        const int mm1 = minc - 1;
        b[msub + minc] = 1.;
        for (int ip = 1; ip <= mm1; ++ip) {
            const int isub = (ip * ip - ip) / 2;
            if (beta[ip] <= 0.) {
                *m = minc - 1;
                return true;
            }
            double gam = 0.;
            for (int j = 1; j <= ip; ++j) {
                gam += cc[j + 1] * b[isub + j];
            }
            gam /= beta[ip];
            for (int jp = 1; jp <= ip; ++jp) {
                b[msub + jp] -= gam * b[isub + jp];
            }
        }
        beta[minc] = 0.;
        for (int j = 1; j <= minc; ++j) {
            beta[minc] += cc[j + 1] * b[msub + j];
        }
        if (beta[minc] <= 0.) {
            *m = minc - 1;
            return true;
        }
        double s = 0.;
        for (int ip = 1; ip <= minc; ++ip) {
            s += cc[ip] * y[ip - 1];
        }
        grc[minc] = -s / beta[minc];
        for (int ip = 1; ip < minc; ++ip) {
            const int m2 = msub + ip;
            y[ip] += grc[minc] * b[m2];
        }
        y[minc] = grc[minc];
        s = grc[minc] * grc[minc] * beta[minc];
        *alpha -= s;
        if (*alpha <= 0.) {
            if (minc < *m) *m = minc;
            return true;
        }
    }
    return true;
}