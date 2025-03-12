#ifndef REFORMANT_PROCESSING_ROUTINES_ECKF_ECKF_H
#define REFORMANT_PROCESSING_ROUTINES_ECKF_ECKF_H

#include <vector>
#include <fftw3.h>

class ECKF {
public:
    ECKF();

    void setup(int blockSize);

    void one_block(const std::vector<double>& y, double fs, double c,
                   double timeToWait, int nPeaks, int nSemitones,
                   std::vector<double>& f0, std::vector<double>& amp);

private:
    /* Kalman filter variables */
    std::vector<double> K; // (3,1)
    int flag; // -1
    double Kthres; // 0.01
    std::vector<double> x_est; // (1,y)
    std::vector<double> amp; // (1,y)
    std::vector<double> phase; // (1,y)
    std::vector<double> Q; // (1,y)
    std::vector<double> f0; // (1,y)
    std::vector<double> P_track; // (1,y)
    std::vector<double> K_track; // (1,y)

    /* State of current frame - initially silent */
    bool silent_cur; // true
    double cur_spf;
    bool harm_prev; // false

    std::vector<double> y_prev;
    std::vector<double> spf;

    /* Silent frame classification */
    bool is_silent(const std::vector<double>& x);

    /* Internal state for pwelch */
    fftwf_plan pwelch_plan;
    std::vector<double> w;
    std::vector<float> psdw;

    void pwelch(const std::vector<double>& x);

    /* Internal state for harmonic_change_detector */
    struct HarmReturn {
        bool flag_cur;
        double f0_est;
        double amp;
        double phase;
    };

    HarmReturn harmonic_change_detector(const std::vector<double>& x_prev,
                                        const std::vector<double>& x_cur, double fs,
                                        int nPeaks,
                                        int nSemitones);
};


#endif // REFORMANT_PROCESSING_ROUTINES_ECKF_ECKF_H
