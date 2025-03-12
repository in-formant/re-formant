#ifndef REFORMANT_PROCESSING_CONTROLLER_PITCH_CREPE_H
#define REFORMANT_PROCESSING_CONTROLLER_PITCH_CREPE_H

#include "../../../state.h"
#include "../../resampler.h"
#include "../../routines/crepe/CREPE.h"
#include "cqueue.hpp"
#include <stv/ShortTermViterbi.hpp>

namespace reformant {
class CREPE {
public:
    explicit CREPE(AppState& appState);

    void reset();

    void process(int& lastTime, std::vector<double>& times,
                 std::vector<double>& pitches, std::vector<double>& salience);

private:
    /* Update the short-term Viterbi object with a new observation frame. */
    void updateViterbiWithNewObservation(int salienceMaxBin,
                                         std::vector<double>& pitches);

    AppState& appState;

    ::CREPE m_crepe;

    Resampler m_16kResampler;
    double m_lastSampleRate;

    gto::cqueue<float> m_16kBlock;
    int m_16kSamplesWaiting; // Number of samples in 16kBlock waiting to be processed.

    /* Keeping entries whose local Viterbi paths haven't converged yet.
       (as defined with short-term viterbi) */
    stv::ShortTermViterbi m_viterbi;

    /* Indices in the times/pitches track. This assumes they aren't modified elsewhere. */
    gto::cqueue<int> m_salienceTrackIndices;

    /* Salience vectors for calculating frequency from bin. */
    gto::cqueue<std::vector<float> > m_salienceVectors;
};
}

#endif // REFORMANT_PROCESSING_CONTROLLER_PITCH_CREPE_H
