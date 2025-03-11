#ifndef REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMOBLOCK_H
#define REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMOBLOCK_H

#include <chrono>
#include <vector>

namespace reformant {

using std::chrono::steady_clock;
using steady_time_point = std::chrono::time_point<steady_clock>;

class FFTMemoBlock {
   public:
    FFTMemoBlock(int windows, int nfft);

    float* windowAt(int index);

    uint64_t millisSince(const steady_time_point& ref) const;

   private:
    void compress();
    void decompress();

    int m_windows;
    int m_nfft;

    std::vector<float> m_data;

    steady_time_point m_lastAccessTime;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMO_H
