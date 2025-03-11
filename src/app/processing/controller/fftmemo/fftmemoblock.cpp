#include "fftmemoblock.h"

#include <lz4.h>

#include <chrono>
#include <ratio>

using namespace reformant;

FFTMemoBlock::FFTMemoBlock(int windows, int nfft) : m_windows(windows), m_nfft(nfft) {}

float* FFTMemoBlock::windowAt(int index) {
    m_lastAccessTime = steady_clock::now();
    return &m_data[index * m_nfft];
}

uint64_t FFTMemoBlock::millisSince(const steady_time_point& ref) const {
    using Ms = std::chrono::duration<uint64_t, std::milli>;
    return std::chrono::duration_cast<Ms>(ref - m_lastAccessTime).count();
}

void FFTMemoBlock::compress() {
    int compressBound =
        LZ4_compressBound(m_windows * m_nfft * sizeof(float) / sizeof(char));
    std::vector<float> m_outdata;
}
void decompress();