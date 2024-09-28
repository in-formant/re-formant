#ifndef REFORMANT_PROCESSING_WAVEFORMCONTROLLER_H
#define REFORMANT_PROCESSING_WAVEFORMCONTROLLER_H

#include <mutex>
#include <vector>
#include <span>

namespace reformant {
struct AppState;

enum WaveformDataType {
    WaveformDataType_Samples,
    WaveformDataType_MinMax,
    WaveformDataType_MinMaxRMS,
};

struct WaveformResults {
    double timeMin;
    double timeMax;
    double timeScale;

    WaveformDataType type;

    std::vector<float> times;
    std::vector<float> samples;
    bool minmaxShaded;
    std::vector<float> mins, maxs;
    std::vector<float> rms1, rms2;
};

class WaveformController final {
public:
    explicit WaveformController(AppState& appState);

    ~WaveformController();

    void forceClear(bool lock = true);

    void updateIfNeeded();

    const WaveformResults& getWaveformForRange(double timeMin, double timeMax,
                                               double tpp);

private:
    void updateWaveformResults();

    void resetWaveformResults();

    AppState& appState;

    std::mutex m_mutex;

    double m_lastSampleRate;

    WaveformResults m_waveResults;

    bool m_needWaveUpdate;
    double m_waveTimeMin;
    double m_waveTimeMax;
    double m_waveTimePerPixel;
};
} // namespace reformant

#endif  // REFORMANT_PROCESSING_WAVEFORMCONTROLLER_H
