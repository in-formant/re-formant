#include <iostream>

#include "routines.h"

int dlpcwtd(const std::vector<double>& s, int ls, std::vector<double>& p, int np,
            std::vector<double>& c, std::vector<double>& phi, std::vector<double>& shi,
            double xl, const std::vector<double>& w) {
    double pss;
    dcwmtrx(s, np, ls, np, phi, shi, &pss, w);

    const int np1 = np + 1;

    if (xl >= 1.0e-4) {
        for (int i = 0; i < np; ++i) {
            p[i] = phi[i * np1];
        }
        p[np] = pss;

        const double pss7 = .0000001 * pss;

        double d;
        int mm = dchlsky(phi, np, c, &d);
        if (mm < np)
            std::cerr << "LPCHFA error covariance matrix rank " << mm << std::endl;
        dlwrtrn(phi, np, c, shi);

        double ee = pss;
        double thres = 0.;

        int m;
        for (m = 0; m < mm; ++m) {
            if (phi[0] < thres) break;
            ee -= c[m] * c[m];
            if (ee < thres) break;
            if (ee < pss7) std::cerr << "LPCHFA is losing accuracy" << std::endl;
        }
        if (m != mm)
            std::cerr << "LPCHFA error - inconsistent value of m " << m << std::endl;

        const double pre = ee * xl;
        for (int i = 1; i < np * np; i += np1) {
            int j = i;
            for (int k = i + np - 1; k < np * np; k += np) {
                phi[k] = phi[j];
                j++;
            }
        }

        const double pre3 = .375 * pre;
        const double pre2 = .25 * pre;
        const double pre0 = .0625 * pre;
        int ip = 0;
        for (int i = 0; i < np * np; i += np1) {
            phi[i] = p[ip] + pre3;
            ip++;

            const int i2 = i - np;
            if (i2 > 0) {
                phi[i2] = phi[i - 1] = phi[i2] - pre2;
            }
            const int i3 = i2 - np;
            if (i3 > 0) {
                phi[i3] = phi[i - 2] = phi[i3] + pre0;
            }
        }
        shi[0] -= pre2;
        shi[1] += pre0;
        p[np] = pss + pre3;
    }

    const int m = dcovlpc(phi, shi, p, np, c);
    return m;
}