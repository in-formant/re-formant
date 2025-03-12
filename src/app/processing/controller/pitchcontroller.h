#ifndef REFORMANT_PROCESSING_PITCHCONTROLLER_H
#define REFORMANT_PROCESSING_PITCHCONTROLLER_H

#include <mutex>
#include <vector>

#include "pitch/CREPE.h"
#include "pitch/RAPT.h"

namespace reformant {
struct AppState;

struct PitchResults {
    std::vector<double> times;
    std::vector<double> pitches;
    std::vector<double> saliences;
};

class PitchController {
   public:
    PitchController(AppState& appState);

    void forceClear(bool lock = true);

    void updateIfNeeded();

    PitchResults getPitchesForRange(double timeMin, double timeMax, double tpp);

    double getInterpolatedVoicing(double time) const;

   private:
    AppState& appState;

    std::mutex m_mutex;

    int m_lastTimeRapt;
    int m_lastTimeCrepe;
    double m_lastSampleRate;

    std::vector<double> m_times;
    std::vector<double> m_pitches;
    std::vector<double> m_saliences;

    RAPT m_rapt;
    CREPE m_crepe;
};
}  // namespace reformant

#endif  // REFORMANT_PROCESSING_PITCHCONTROLLER_H