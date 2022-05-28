#include <iostream>
#include <random>

#include "routines.h"

#define MAX_ITS 100   /* Max iterations before trying new starts */
#define MAX_TRYS 100  /* Max number of times to try new starts */
#define MAX_ERR 1.e-6 /* Max acceptable error in quad factor */

/* Random generator */
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> distrib(-0.5, 0.5);

bool lbpoly(std::vector<double>& a, int order, std::vector<double>& rootr,
            std::vector<double>& rooti) {
    /* Rootr and rooti are assumed to contain starting points for the root
       search on entry to lbpoly(). */

    const double lim0 = 0.5 * sqrt(DBL_MAX);

    std::vector<double> b(order + 1);
    std::vector<double> c(order + 1);

    int ord;
    for (ord = order; ord > 2; ord -= 2) {
        const int ordm1 = ord - 1;
        const int ordm2 = ord - 2;

        /* Here is a kluge to prevent UNDERFLOW! (Sometimes the near-zero
           roots left in rootr and/or rooti cause underflow here...	*/
        if (fabs(rootr[ordm1]) < 1.0e-10) rootr[ordm1] = 0.;
        if (fabs(rooti[ordm1]) < 1.0e-10) rooti[ordm1] = 0.;

        double p = -2. * rootr[ordm1]; /* set initial guesses for quad factor */
        double q = (rootr[ordm1] * rootr[ordm1]) + (rooti[ordm1] * rooti[ordm1]);

        int ntrys, itcnt;
        for (ntrys = 0; ntrys < MAX_TRYS; ++ntrys) {
            bool found = false;

            for (itcnt = 0; itcnt < MAX_ITS; ++itcnt) {
                const double lim = lim0 / (1. + fabs(p) + fabs(q));

                b[ord] = a[ord];
                b[ordm1] = a[ordm1] - (p * b[ord]);
                c[ord] = b[ord];
                c[ordm1] = b[ordm1] - (p * c[ord]);

                int k;
                for (k = 2; k <= ordm1; ++k) {
                    const int mmk = ord - k;
                    const int mmkp2 = mmk + 2;
                    const int mmkp1 = mmk + 1;
                    b[mmk] = a[mmk] - (p * b[mmkp1]) - (q * b[mmkp2]);
                    c[mmk] = b[mmk] - (p * c[mmkp1]) - (q * c[mmkp2]);
                    if (b[mmk] > lim || c[mmk] > lim) break;
                }

                if (k > ordm1) { /* normal exit from for(k ... */
                    b[0] = a[0] - p * b[1] - q * b[2];
                    if (b[0] <= lim) k++;
                }
                if (k <= ord) /* Some coefficient exceeded lim */
                    break;

                const double err = fabs(b[0]) + fabs(b[1]);
                if (err <= MAX_ERR) {
                    found = true;
                    break;
                }

                const double den = (c[2] * c[2]) - (c[3] * (c[1] - b[1]));
                if (den == 0.) break;

                const double delp = ((c[2] * b[1]) - (c[3] * b[0])) / den;
                const double delq = ((c[2] * b[0]) - (b[1] * (c[1] - b[1]))) / den;

                p += delp;
                q += delq;
            } /* for itcnt */

            if (found)
                break;
            else { /* try some new starting values */
                p = distrib(gen);
                q = distrib(gen);
            }
        } /* for ntrys */

        if (itcnt >= MAX_ITS && ntrys >= MAX_TRYS) {
            return false;
        }

        if (!qquad(1.0, p, q, &rootr[ordm1], &rooti[ordm1], &rootr[ordm2], &rooti[ordm2]))
            return false;

        /* Update the coefficient array with the coeffs. of the
           reduced polynomial. */
        for (int i = 0; i <= ordm2; i++) a[i] = b[i + 2];
    } /* for ord */

    if (ord == 2) { /* Is the last factor a quadratic? */
        if (!qquad(a[2], a[1], a[0], &rootr[1], &rooti[1], &rootr[0], &rooti[0]))
            return false;
        return true;
    }
    if (ord < 1) {
        std::cerr << "Bad ORDER parameter in lbpoly()" << std::endl;
        return false;
    }

    if (a[1] != 0.)
        rootr[0] = -a[0] / a[1];
    else {
        rootr[0] = 100.; /* arbitrary recovery value */
        std::cerr << "Numerical problems in lbpoly()" << std::endl;
    }
    rooti[0] = 0.;

    return true;
}
