#ifndef REFORMANT_PROCESSING_PITCHCONTROLLER_H
#define REFORMANT_PROCESSING_PITCHCONTROLLER_H

#include <fftw3.h>

#include <mutex>
#include <vector>

#include "../resampler.h"

namespace reformant {

struct AppState;

struct PitchResults {
    std::vector<double> times;
    std::vector<double> pitches;
};

class PitchController {
   public:
    PitchController(AppState& appState);

    void forceClear(bool lock = true);

    void updateIfNeeded();

    PitchResults getPitchesForRange(double timeMin, double timeMax, double tpp);

    double getInterpolatedVoicing(double time);

   private:
    AppState& appState;

    std::mutex m_mutex;

    Resampler m_dsResampler;

    int m_lastTime;
    double m_lastSampleRate;

    std::vector<double> m_times;
    std::vector<double> m_pitches;

    int m_minSilenceRunLength;
    int m_minVoicingRunLength;

    struct PitchPoint {
        double time, pitch;
    };
    std::vector<PitchPoint> m_pitchBuffer;

    double F0min;
    double F0max;
    double cand_tr;
    double lag_wt;
    double freq_wt;
    double vtran_c;
    double vtr_a_c;
    double vtr_s_c;
    double vo_bias;
    double doubl_c;
    double a_fact;
    int n_cands;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_PITCHCONTROLLER_H