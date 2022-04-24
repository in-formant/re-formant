#ifndef REFORMANT_PROCESSING_SPECTROGRAMCONTROLLER_H
#define REFORMANT_PROCESSING_SPECTROGRAMCONTROLLER_H

#include <fftw3.h>

#include <mutex>
#include <vector>

namespace reformant {

struct AppState;

struct SpectrogramResults {
    double timeMin;
    double timeMax;
    double freqMin;
    double freqMax;
    int numSlices;
    int numFreqs;
    std::vector<double> data;
};

class SpectrogramController {
   public:
    SpectrogramController(AppState& appState);
    virtual ~SpectrogramController();

    double time() const;
    void setTime(double time);

    int timeSamples() const;
    void setTimeSamples(int timeSamples);

    int fftLength() const;
    void setFftLength(int nfft);

    uint64_t maxMemoryMemo() const;
    void setMaxMemoryMemo(uint64_t mem);

    void forceClear();

    double approxMemoCapacityInSeconds() const;

    uint64_t bytesUsedByMemo();

    void updateIfNeeded();

    SpectrogramResults getSpectrogramForRange(double timeMin, double timeMax,
                                              double tpp);

   private:
    AppState& appState;

    volatile double m_time;  // volatile because modified from another thread
    volatile int m_timeSamples;

    std::mutex m_fftMutex;

    int m_fftLength;  // non volatile bc only modified from UI thread
    fftwf_plan m_fftPlan;
    float* m_fftInput;
    float* m_fftOutput;

    int m_fftStride;

    uint64_t m_fftMemoMaxMemory;
    double m_fftMemoStartTime;
    int m_fftMemoStartBlock;
    int m_fftMemoRowCount;

    std::vector<double> m_fftMemo;

    double m_lastSpecRequestTimeMin;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_SPECTROGRAMCONTROLLER_H
