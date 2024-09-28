#include "spectrogramcontroller.h"

#include <cfloat>
#include <cmath>
#include <iostream>

#include "../../memusage.h"
#include "../../state.h"

using namespace reformant;

SpectrogramController::SpectrogramController(AppState& appState)
    : appState(appState),
      m_time(0.0),
      m_timeSamples(0),
      m_fftLength(1024),
      m_fftPlan(nullptr),
      m_fftInput(nullptr),
      m_fftOutput(nullptr),
      m_fftMemoMaxMemory(1024_u64 * 1024_u64 * 256_u64),
      m_fftMemoStartTime(0.0),
      m_fftMemoRowCount(0),
      m_needSpecUpdate(false) {
}

SpectrogramController::~SpectrogramController() {
    if (m_fftPlan != nullptr) fftwf_destroy_plan(m_fftPlan);
    if (m_fftInput != nullptr) fftwf_free(m_fftInput);
    if (m_fftOutput != nullptr) fftwf_free(m_fftOutput);
}

double SpectrogramController::time() const { return m_time; }

void SpectrogramController::setTime(double time) {
    m_time = time;
    m_timeSamples = time * appState.audioTrack.sampleRate();
}

int SpectrogramController::timeSamples() const { return m_timeSamples; }

void SpectrogramController::setTimeSamples(int timeSamples) {
    m_timeSamples = timeSamples;
    m_time = timeSamples / appState.audioTrack.sampleRate();
}

int SpectrogramController::fftLength() const { return m_fftLength; }

void SpectrogramController::setFftLength(int nfft) {
    std::lock_guard lockGuard(m_fftMutex);

    if (m_fftPlan != nullptr) fftwf_destroy_plan(m_fftPlan);
    if (m_fftInput != nullptr) fftwf_free(m_fftInput);
    if (m_fftOutput != nullptr) fftwf_free(m_fftOutput);

    m_fftLength = nfft;
    m_fftInput = fftwf_alloc_real(nfft);
    m_fftOutput = fftwf_alloc_real(nfft);
    m_fftPlan = fftwf_plan_r2r_1d(nfft, m_fftInput, m_fftOutput, FFTW_R2HC, FFTW_MEASURE);

    m_fftMemo.clear();

    m_fftMemoRowCount = 0;
}

uint64_t SpectrogramController::maxMemoryMemo() const { return m_fftMemoMaxMemory; }

void SpectrogramController::setMaxMemoryMemo(uint64_t mem) {
    m_fftMemoMaxMemory = mem;
    m_fftMemo.reserve(mem / sizeof(decltype(m_fftMemo)::size_type));
}

void SpectrogramController::forceClear() {
    std::lock_guard lockGuard(m_fftMutex);
    m_fftMemo.clear();
    m_fftMemoRowCount = 0;
}

double SpectrogramController::approxMemoCapacityInSeconds() const {
    const uint64_t numElem =
        (m_fftMemoMaxMemory - sizeof(m_fftMemo)) / sizeof(decltype(m_fftMemo)::size_type);
    const uint64_t numFreqs = m_fftLength / 2;
    const double blockRate = appState.audioTrack.sampleRate() / m_fftStride;
    return (numElem / numFreqs) / blockRate;
}

uint64_t SpectrogramController::bytesUsedByMemo() {
    return m_fftMemo.capacity() * sizeof(decltype(m_fftMemo)::size_type);
}

void SpectrogramController::updateIfNeeded() {
    using namespace std::chrono_literals;

    // Just return if we couldn't lock, this isn't important because it's
    // ran periodically, and it avoids a potential deadlock.
    const std::unique_lock trackLock(appState.audioTrack.mutex(), 50ms);
    if (!trackLock.owns_lock()) return;

    std::lock_guard fftGuard(m_fftMutex);

    // Check memory usage for max memory cap.
    if (bytesUsedByMemo() > m_fftMemoMaxMemory) {
        // Clear current memory.
        m_fftMemo.clear();
        m_fftMemo.shrink_to_fit();

        m_fftMemoRowCount = 0;

        // Set the starting time related to the last requested time.
        m_fftMemoStartTime = std::max(0.0, m_specTimeMin - 10.0);
    }

    if (m_needSpecUpdate) {
        updateSpectrogramResults();
        m_needSpecUpdate = false;
    }

    // The FFT memo is a series of blocks of (NFFT/2)-length spectra,
    // each representing the spectrum of NFFT-length windows,
    // each spaced 10ms apart, starting from t=0.

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSampleCount = appState.audioTrack.sampleCount();

    // Stride size in samples.
    m_fftStride = m_fftLength / 4;
    //m_fftStride = static_cast<int>(std::round(20.0 / 1000.0 * sampleRate));

    // How many blocks we're expecting for this given stride.
    const int numBlocks = (trackSampleCount - m_fftLength) / m_fftStride;
    const int numFreqs = m_fftLength / 2;

    m_fftMemoStartBlock = static_cast<int>(std::floor(
        m_fftMemoStartTime * sampleRate / m_fftStride));

    // How many blocks in the memo, accounting for starting block number.
    const int actualNumBlocks = numBlocks - m_fftMemoStartBlock;

    // Check how many more blocks we need to calculate.
    const int numMissingBlocks = actualNumBlocks - m_fftMemoRowCount;

    // We don't need to calculate anything, return now.
    if (numMissingBlocks <= 0) {
        return;
    }

    m_fftMemo.resize(actualNumBlocks * numFreqs);

    m_fftMemoRowCount = actualNumBlocks;

    // Get the starting slice & index where we need to start.
    int slice = actualNumBlocks - numMissingBlocks;
    int index = (m_fftMemoStartBlock + slice) * m_fftStride;

    for (; slice < actualNumBlocks; ++slice, index += m_fftStride) {
        // Get the track samples.
        auto trackSamples = appState.audioTrack.data(index, m_fftLength);

        // Apply windowing.
        const int N = m_fftLength - 1;
        for (int i = 0; i < m_fftLength; ++i) {
            // Blackman-Nuttall
            constexpr double a0 = 0.3635819;
            constexpr double a1 = 0.4891775;
            constexpr double a2 = 0.1365995;
            constexpr double a3 = 0.0106411;

            const double wk = a0 - a1 * cos((2 * M_PI * i) / N) +
                              a2 * cos((4 * M_PI * i) / N) - a3 * cos((6 * M_PI * i) / N);

            trackSamples[i] = static_cast<float>(trackSamples[i] * wk);
        }

        // Compute FFT.
        std::copy(trackSamples.begin(), trackSamples.end(), m_fftInput);
        fftwf_execute(m_fftPlan);

        // Compute spectrum from FFT output.
        for (int i = 0; i < numFreqs; ++i) {
            const double real = m_fftOutput[i + 1];
            const double imag = (i > 0) ? m_fftOutput[m_fftLength - 1 - i] : 0;
            const double mag = real * real + imag * imag;

            const double magDb = 20.0 * log10(mag <= 0 ? DBL_EPSILON : mag);

            m_fftMemo[i + slice * numFreqs] = static_cast<float>(magDb);
        }
    }
}

const SpectrogramResults& SpectrogramController::getSpectrogramForRange(
    const double timeMin, const double timeMax, const double timePerPixel) {
    // This method will be called from the UI thread, we don't want that.
    // So let's queue updating results with those parameters so it gets recalculated on updateIfNeeded.

    std::lock_guard lockGuard(m_fftMutex);

    // Extend by some windows on each side.
    const double sampleRate = appState.audioTrack.sampleRate();
    const double oneWindow = m_fftLength / sampleRate;

    m_needSpecUpdate = true;
    m_specTimeMin = timeMin - 4 * oneWindow;
    m_specTimeMax = timeMax + 4 * oneWindow;
    m_specTimePerPixel = timePerPixel;

    return m_specResults;
}

void SpectrogramController::updateSpectrogramResults() {
    auto& spec = m_specResults;
    const double timeMin = m_specTimeMin;
    const double timeMax = m_specTimeMax;
    const double timePerPixel = m_specTimePerPixel;

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Half-window in seconds.
    const double halfWindow = (.5 * m_fftLength) / sampleRate;

    // Set the frequency limits.
    spec.freqMin = 1 / sampleRate;
    spec.freqMax = sampleRate / 2;
    spec.numFreqs = m_fftLength / 2;

    // Stride size in seconds.
    const double stride = m_fftStride / sampleRate;

    // Find integer multiple to downsample per timePerPixel.
    const int dsMult = std::max(1, static_cast<int>(std::ceil(timePerPixel / stride)));

    // Stride size in samples.
    const double blockRate = sampleRate / m_fftStride;

    // Number of blocks (slices) in the FFT memo.
    const int numBlocks = m_fftMemoRowCount;

    // Find the first block index from timeMin (can be out of bounds)
    const int minBlock = ((int)std::floor(timeMin * blockRate) - m_fftMemoStartBlock);

    // Find the last block index from timeMax (can be out of bounds)
    const int maxBlock = ((int)std::ceil(timeMax * blockRate) - m_fftMemoStartBlock);

    // Find the first block index that's in the FFT memo.
    const int startBlockUndec = std::max(minBlock, 0);
    const int endBlockUndec = std::min(maxBlock, numBlocks - 1);

    // Round to the nearest dsMult multiple
    const int startBlock = startBlockUndec / dsMult * dsMult;
    const int endBlock = endBlockUndec / dsMult * dsMult;

    if (endBlock < startBlock) {
        spec.timeMin = 0;
        spec.timeMax = 0;
        spec.numSlices = 0;
        return;
    }

    // Set the time limits. Shift them to align the middle.
    spec.timeMin = (startBlock + m_fftMemoStartBlock) / blockRate + halfWindow;
    spec.timeMax = (endBlock + m_fftMemoStartBlock) / blockRate + halfWindow;

    // Number of blocks (slices) in the requested data.
    spec.numSlices = (endBlock - startBlock) / dsMult;

    // Resize.
    spec.data.resize(spec.numSlices * spec.numFreqs);
    spec.dataColMajor.resize(spec.numSlices * spec.numFreqs);

    // Copy the data from FFT memo to result array.
    int block = startBlock;

    for (int slice = 0; slice < spec.numSlices; ++slice) {
        const int memoIndex = block * spec.numFreqs;

        std::copy_n(m_fftMemo.begin() + memoIndex, spec.numFreqs,
                    spec.dataColMajor.begin() + slice * spec.numFreqs);
        block += dsMult;
    }

    for (int i = 0; i < spec.numFreqs; ++i) {
        for (int slice = 0; slice < spec.numSlices; ++slice) {
            spec.data[slice + i * spec.numSlices] = spec.dataColMajor[
                i + slice * spec.numFreqs];
        }
    }
}