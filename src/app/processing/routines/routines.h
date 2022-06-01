#ifndef REFORMANT_PROCESSING_ROUTINES_ROUTINES_H
#define REFORMANT_PROCESSING_ROUTINES_ROUTINES_H

#include <cmath>
#include <vector>

inline double integerize(const double time, const double freq) {
    const int i = (int)(.5 + (freq * time));
    return ((double)i) / freq;
}

inline int eround(const double flnum) {
    return (flnum >= 0.0) ? (int)(flnum + .5) : (int)(flnum - .5);
}

enum WindowType {
    WINDOW_RECTANGULAR,
    WINDOW_HAMMING,
    WINDOW_COS4,
    WINDOW_HANN,
};

enum LpcType {
    LPC_AUTOC,
    LPC_BSA,
    LPC_COVAR,
};

struct Pole {
    int offset;
    double rms;               /* rms for current LPC analysis frame */
    double rms2;              /* rms for current F0 analysis frame */
    double f0;                /* fundamental frequency estimate for this frame */
    double pv;                /* probability that frame is voiced */
    double change;            /* spec. distance between current and prev. frames */
    int npoles;               /* # of complex poles from roots of LPC polynomial */
    std::vector<double> freq; /* array of complex pole frequencies (Hz) */
    std::vector<double> band; /* array of complex pole bandwidths (Hz) */
};

struct PoleArray {
    std::vector<Pole> pole;
    int length;
    double sampleRate;
};

inline constexpr int MAXORDER = 60;

PoleArray lpc_poles(const std::vector<double>& data, double sampleRate,
                    double windowDuration, double frameInterval, int lpcOrder,
                    double preEmphasis, LpcType lpcType, WindowType windowType);

void dpform(const std::vector<Pole>& poles, int nform, double nomF1);

void rwindow(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
             double preEmphasis);

void hwindow(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
             double preEmphasis);

void cwindow(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
             double preEmphasis);

void hnwindow(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
              double preEmphasis);

void w_window(const std::vector<double>& in, int ioff, std::vector<double>& out, int n,
              double preEmphasis, WindowType type);

void autoc(int windowSize, const std::vector<double>& s, int p, std::vector<double>& r,
           double* e);

void durbin(const std::vector<double>& r, std::vector<double>& k, std::vector<double>& a,
            int p, double* ex);

bool lpc(int np, double lpcStabl, int wind, const std::vector<double>& data, int dataOff,
         std::vector<double>& lpc, double* energy, double preEmphasis,
         WindowType windowType);

bool lpcbsa(int np, double lpcStabl, int wind, const std::vector<double>& data,
            int dataOff, std::vector<double>& lpc, double* energy, double preEmphasis);

bool w_covar(const std::vector<double>& data, int dataOff, int* m, int n, int istrt,
             std::vector<double>& y, double* alpha, double* r0, double preEmphasis,
             WindowType windowType);

void dlwrtrn(const std::vector<double>& a, int n, std::vector<double>& x,
             const std::vector<double>& y);

void dreflpc(const std::vector<double>& c, std::vector<double>& a, int n);

int dchlsky(std::vector<double>& a, int n, std::vector<double>& t, double* det);

int dcovlpc(std::vector<double>& p, const std::vector<double>& s, std::vector<double>& a,
            int n, std::vector<double>& c);

void dcwmtrx(const std::vector<double>& s, int ni, int nl, int np,
             std::vector<double>& phi, std::vector<double>& shi, double* ps,
             const std::vector<double>& w);

int dlpcwtd(const std::vector<double>& s, int ls, std::vector<double>& p, int np,
            std::vector<double>& c, std::vector<double>& phi, std::vector<double>& shi,
            double* xl, const std::vector<double>& w);

bool qquad(double a, double b, double c, double* r1r, double* r1i, double* r2r,
           double* r2i);

bool lbpoly(std::vector<double>& a, int order, std::vector<double>& rootr,
            std::vector<double>& rooti);

bool formant(int lpcOrder, double sFreq, std::vector<double>& lpca, int* nForm,
             std::vector<double>& freq, std::vector<double>& band, bool init);

#endif  // REFORMANT_PROCESSING_ROUTINES_ROUTINES_H