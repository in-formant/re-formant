#include "fftmemo.h"

using namespace reformant;

FFTMemo::FFTMemo(int windowsPerBlock, int nfft)
    : m_windowsPerBlock(windowsPerBlock), m_nfft(nfft) {
    m_blocks.emplace_back(windowsPerBlock, nfft);
}