#ifndef REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMO_H
#define REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMO_H

#include <vector>

#include "fftmemoblock.h"

namespace reformant {

/*
Fixed length chunks of consecutive FFT windows.
*/
class FFTMemo {
   public:
    FFTMemo(int windowsPerBlock, int nfft);

   private:
    int m_windowsPerBlock;
    int m_nfft;

    std::vector<FFTMemoBlock> m_blocks;
};

}  // namespace reformant

#endif  // REFORMANT_PROCESSING_CONTROLLER_FFTMEMO_FFTMEMO_H