#ifndef REFORMANT_PROCESSING_CONTROLLER_PITCH_RAPT_H
#define REFORMANT_PROCESSING_CONTROLLER_PITCH_RAPT_H

#include "../../../state.h"
#include "../../resampler.h"

namespace reformant {
class RAPT {
public:
    explicit RAPT(AppState& appState);

    void reset();

    void process(int& lastTime, std::vector<double>& times,
                 std::vector<double>& pitches);

private:
    AppState& appState;

    Resampler m_dsResampler;

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
} //namespace reformant

#endif //REFORMANT_PROCESSING_CONTROLLER_PITCH_RAPT_H
