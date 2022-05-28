#include <iostream>
#include <random>

#include "routines.h"

/* Random generator */
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> distrib(-1.0, 1.0);

bool lpcbsa(int np, double lpcStabl, int wind, const std::vector<int>& data, int dataOff,
            std::vector<double>& lpc, double* energy, double preEmphasis) {
    static std::vector<double> w;

    if (w.size() != wind) {
        /* need to compute a new window? */
        const double fham = 6.28318506 / wind;
        w.resize(wind);
        for (int i = 0; i < wind; ++i) {
            w[i] = .54 - .46 * cos(i * fham);
        }
    }
    int wind1;
    wind += np + 1;
    wind1 = wind - 1;

    std::vector<double> sig(wind);
    for (int i = 0; i < wind; ++i) {
        sig[i] = data[i] + .016 * distrib(gen) - .008;
    }
    for (int i = 1; i < wind; ++i) {
        sig[i - 1] = sig[i] - preEmphasis * sig[i - 1];
    }

    double amax = 0;
    for (int i = np; i < wind1; ++i) {
        amax += sig[i] * sig[i];
    }
    *energy = sqrt(amax / double(wind));
    amax = 1.0 / *energy;

    for (int i = 0; i < wind1; ++i) {
        sig[i] *= amax;
    }

    std::vector<double> rc(np);
    std::vector<double> phi(np * np);
    std::vector<double> shi(np);
    double xl = .09;

    int mm = dlpcwtd(sig, wind1, lpc, np, rc, phi, shi, &xl, w);
    if (mm != np) {
        std::cerr << "lpcwtd error mm < np " << mm << " " << np << std::endl;
        return false;
    }
    return false;
}