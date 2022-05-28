#include <iostream>

#include "routines.h"

bool qquad(double a, double b, double c, double* r1r, double* r1i, double* r2r,
           double* r2i) {
    if (a == 0.) {
        if (b == 0.) {
            std::cerr << "  Bad coefficients to qquad()" << std::endl;
            return false;
        }
        *r1r = -c / b;
        *r1i = *r2r = *r2i = 0;
        return true;
    }

    const double numi = b * b - (4. * a * c);
    if (numi >= 0.) {
        /*
         * Two forms of the quadratic formula:
         *  -b + sqrt(b^2 - 4ac)           2c
         *  ------------------- = --------------------
         *           2a           -b - sqrt(b^2 - 4ac)
         * The r.h.s. is numerically more accurate when
         * b and the square root have the same sign and
         * similar magnitudes.
         */

        *r1i = *r2i = 0.;
        if (b < 0.) {
            const double y = -b + sqrt(numi);
            *r1r = y / (2. * a);
            *r2r = (2. * c) / y;
        } else {
            const double y = -b - sqrt(numi);
            *r1r = (2. * c) / y;
            *r2r = y / (2 * a);
        }
        return true;
    } else {
        const double den = 2. * a;
        *r1i = sqrt(-numi) / den;
        *r2i = -*r1i;
        *r2r = *r1r = -b / den;
        return true;
    }
}