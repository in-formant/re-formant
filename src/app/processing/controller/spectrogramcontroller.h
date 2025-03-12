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
    std::vector<float> data;
    std::vector<float> dataColMajor;
};

class SpectrogramController final {
public:
    SpectrogramController(AppState& appState);

    ~SpectrogramController();

    double time() const;

    void setTime(double time);

    [[nodiscard]] int timeSamples() const;

    void setTimeSamples(int timeSamples);

    [[nodiscard]] int fftLength() const;

    void setFftLength(int nfft);

    [[nodiscard]] uint64_t maxMemoryMemo() const;

    void setMaxMemoryMemo(uint64_t mem);

    void forceClear();

    [[nodiscard]] double approxMemoCapacityInSeconds() const;

    uint64_t bytesUsedByMemo();

    void updateIfNeeded();

    const SpectrogramResults& getSpectrogramForRange(double timeMin, double timeMax,
                                                     double tpp);
<<<<<<< HEAD
=======

private:
    void updateSpectrogramResults();
>>>>>>> ad5d6c670eab97383613c8523ec32898a1ef1cc9

    AppState& appState;

    volatile double m_time; // volatile because modified from another thread
    volatile int m_timeSamples;

    std::mutex m_fftMutex;

    int m_fftLength; // nonvolatile bc only modified from UI thread
    fftwf_plan m_fftPlan;
    float* m_fftInput;
    float* m_fftOutput;

    int m_fftStride;

    uint64_t m_fftMemoMaxMemory;
    double m_fftMemoStartTime;
    int m_fftMemoStartBlock;
    int m_fftMemoRowCount;

    std::vector<float> m_fftMemo;

<<<<<<< HEAD
    double m_lastSpecRequestTimeMin;
    SpectrogramResults m_specResults;
=======
    SpectrogramResults m_specResults;

    bool m_needSpecUpdate;
    double m_specTimeMin;
    double m_specTimeMax;
    double m_specTimePerPixel;
>>>>>>> ad5d6c670eab97383613c8523ec32898a1ef1cc9
};
} // namespace reformant

#endif  // REFORMANT_PROCESSING_SPECTROGRAMCONTROLLER_H
