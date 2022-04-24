#include "spectrogramcontroller.h"

#include <cfloat>
#include <cmath>
#include <iostream>

#include "../memusage.h"
#include "../state.h"

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
      m_lastSpecRequestTimeMin(0.0) {}

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
    m_fftPlan = fftwf_plan_r2r_1d(nfft, m_fftInput, m_fftOutput, FFTW_R2HC,
                                  FFTW_MEASURE);

    m_fftMemo.clear();

    m_fftMemoRowCount = 0;
}

uint64_t SpectrogramController::maxMemoryMemo() const {
    return m_fftMemoMaxMemory;
}

void SpectrogramController::setMaxMemoryMemo(uint64_t mem) {
    m_fftMemoMaxMemory = mem;
}

void SpectrogramController::forceClear() {
    std::lock_guard lockGuard(m_fftMutex);
    m_fftMemo.clear();
    m_fftMemoRowCount = 0;
}

double SpectrogramController::approxMemoCapacityInSeconds() const {
    const uint64_t numElem = (m_fftMemoMaxMemory - sizeof(m_fftMemo)) /
                             sizeof(decltype(m_fftMemo)::size_type);
    const uint64_t numFreqs = m_fftLength / 2;
    const double blockRate = appState.audioTrack.sampleRate() / m_fftStride;
    return (numElem / numFreqs) / blockRate;
}

uint64_t SpectrogramController::bytesUsedByMemo() {
    return sizeof(m_fftMemo) +
           m_fftMemo.capacity() * sizeof(decltype(m_fftMemo)::size_type);
}

void SpectrogramController::updateIfNeeded() {
    using namespace std::chrono_literals;

    // Just return if we couldn't lock, this isn't important because it's
    // ran periodically, and it avoids a potential deadlock.
    std::unique_lock trackLock(appState.audioTrack.mutex(), 50ms);
    if (!trackLock.owns_lock()) return;

    std::lock_guard fftGuard(m_fftMutex);

    // Check memory usage for max memory cap.
    if (bytesUsedByMemo() > m_fftMemoMaxMemory) {
        // Clear current memory.
        m_fftMemo.clear();
        m_fftMemo.shrink_to_fit();

        m_fftMemoRowCount = 0;

        // Set the starting time related to the last requested time.
        m_fftMemoStartTime = std::max(0.0, m_lastSpecRequestTimeMin - 10.0);
    }

    // The FFT memo is a series of blocks of (NFFT/2)-length spectra,
    // each representing the spectrum of NFFT-length windows,
    // each spaced 10ms apart, starting from t=0.

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    // Stride size in samples.
    m_fftStride = (int)std::round(10.0 / 1000.0 * sampleRate);

    // How many blocks we're expecting for this given stride.
    const int numBlocks = (trackSamples - m_fftLength) / m_fftStride;
    const int numFreqs = m_fftLength / 2;

    m_fftMemoStartBlock =
        (int)std::floor(m_fftMemoStartTime * sampleRate / m_fftStride);

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
        for (int i = 0; i < m_fftLength; ++i) {
            // Blackman - Nuttall
            constexpr double a0 = 0.3635819;
            constexpr double a1 = 0.4891775;
            constexpr double a2 = 0.1365995;
            constexpr double a3 = 0.0106411;

            trackSamples[i] *= a0 -
                               a1 * cos((2 * M_PI * i) / (m_fftLength - 1)) +
                               a2 * cos((4 * M_PI * i) / (m_fftLength - 1)) -
                               a3 * cos((6 * M_PI * i) / (m_fftLength - 1));
        }

        // Compute FFT.
        std::copy(trackSamples.begin(), trackSamples.end(), m_fftInput);
        fftwf_execute(m_fftPlan);

        // Compute spectrum from FFT output.
        for (int i = 0; i < numFreqs; ++i) {
            const double real = m_fftOutput[i + 1];
            const double imag = (i > 0) ? m_fftOutput[m_fftLength - 1 - i] : 0;
            const double mag = real * real + imag * imag;

            m_fftMemo[i + slice * numFreqs] = mag;
        }
    }
}

SpectrogramResults SpectrogramController::getSpectrogramForRange(
    const double timeMin, const double timeMax, const double timePerPixel) {
    std::lock_guard lockGuard(m_fftMutex);

    SpectrogramResults spec;

    // Track sample rate.
    const double sampleRate = appState.audioTrack.sampleRate();

    // Set the frequency limits.
    spec.freqMin = 1 / sampleRate;
    spec.freqMax = sampleRate / 2;
    spec.numFreqs = m_fftLength / 2;

    // Store the last requested timeMin to inform a potential memo
    // reallocation.
    m_lastSpecRequestTimeMin = timeMin;

    // Track length in samples.
    const int trackSamples = appState.audioTrack.sampleCount();

    // Stride size in seconds.
    const double stride = m_fftStride / sampleRate;

    // Find integer multiple to downsample per timePerPixel.
    const int dsMult = std::max(1, (int)std::floor(timePerPixel / stride) - 1);

    // Stride size in samples.
    const double blockRate = sampleRate / m_fftStride;

    // Number of blocks (slices) in the FFT memo.
    const int numBlocks = m_fftMemoRowCount;

    // Find the first block index from timeMin (can be out of bounds)
    const int minBlock =
        ((int)std::floor(timeMin * blockRate) - m_fftMemoStartBlock);

    // Find the last block index from timeMax (can be out of bounds)
    const int maxBlock =
        ((int)std::ceil(timeMax * blockRate) - m_fftMemoStartBlock);

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
        return spec;
    }

    // Set the time limits.
    spec.timeMin = (startBlock + m_fftMemoStartBlock) / blockRate;
    spec.timeMax = (endBlock + m_fftMemoStartBlock) / blockRate;

    // Number of blocks (slices) in the requested data.
    spec.numSlices = (endBlock - startBlock) / dsMult;

    // Initialize the data to zero.
    spec.data.resize(spec.numSlices * spec.numFreqs, 0.0);

    // Copy the data from col-major FFT memo to row-major result array.
    int slice = 0;
    int block = startBlock;

    while (block < endBlock) {
        for (int i = 0; i < spec.numFreqs; ++i) {
            const int index = (spec.numFreqs - 1 - i) * spec.numSlices + slice;
            // Fill with zeroes if not in memo.
            const int memoIndex = i + block * spec.numFreqs;
            if (memoIndex < m_fftMemo.size()) {
                spec.data[index] = m_fftMemo[memoIndex];
            } else {
                spec.data[index] = 0;
            }
        }
        ++slice;
        block += dsMult;
    }

    for (auto& mag : spec.data) {
        mag = 20.0 * log10(mag <= 0 ? DBL_EPSILON : mag);
    }

    return spec;
}