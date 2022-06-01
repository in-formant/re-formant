#include "routines.h"

int dchlsky(std::vector<double>& a, int n, std::vector<double>& t, double* det) {
    *det = 1.;

    int m = 0;
    for (int i = 0; i < n * n; i += n) {
        int k = i;
        int s = 0;
        for (int j = 0; j <= i; j += n) {
            double sm = a[k];
            for (int l = i; l < k; ++l) {
                sm -= a[l] * a[j + l - i];
            }
            if (i == j) {
                if (sm <= 0.) return m;
                t[s] = sqrt(sm);
                *det *= t[s];
                a[k] = t[s];
                t[s] = 1. / t[s];
                k++;
                m++;
                s++;
            } else {
                a[k] = sm * t[s];
                k++;
                s++;
            }
        }
    }
    return m;
}