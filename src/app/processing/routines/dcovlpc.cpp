#include <iostream>

#include "routines.h"

int dcovlpc(std::vector<double>& p, const std::vector<double>& s, std::vector<double>& a,
            int n, std::vector<double>& c) {
    constexpr double thres = 1.0e-31;

    double d;
    int m = dchlsky(p, n, c, &d);
    dlwrtrn(p, n, c, s);

    const int nm = n * m;

    m = 0;
    for (int i = 0; i < nm; i += n + 1) {
        if (p[i] < thres) break;
        m++;
    }

    const double ps = a[n];
    const double ps1 = 1.0e-8 * ps;

    double ee = a[n];
    int l;
    for (l = 0; l < m; ++l) {
        ee -= c[l] * c[l];
        if (ee < thres) break;
        if (ee < ps1) std::cerr << "covlpc is losing accuracy" << std::endl;
        a[l] = sqrt(ee);
    }
    m = l;
    c[0] = -c[0] / sqrt(ps);
    for (int i = 1; i < m; ++i) {
        c[i] = -c[i] / a[i - 1];
    }
    dreflpc(c, a, m);
    for (int i = m + 1; i <= n; ++i) {
        a[i] = 0.;
    }
    return m;
}