#ifndef REFORMANT_PROCESSING_FORMANTCONTROLLER_H
#define REFORMANT_PROCESSING_FORMANTCONTROLLER_H

#include <fftw3.h>

#include <mutex>
#include <vector>

#include "../resampler.h"
#include "formants.h"

namespace reformant {

struct AppState;

struct FormantResults {
    std::vector<double> times;
    std::vector<double> frequencies;
};

class FormantController {
   public:
    FormantController(AppState& appState);

    void forceClear(bool lock = true);

    void updateIfNeeded();

    FormantResults getFormantsForRange(double timeMin, double timeMax, double tpp);

   private:
    AppState& appState;

    std::mutex m_mutex;

    Resampler m_dsResampler;

    int m_lastTime;
    double m_lastSampleRate;

    std::vector<double> m_times;
    std::vector<double> m_frequencies;

    FormantTracking m_tracking;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_FORMANTCONTROLLER_H