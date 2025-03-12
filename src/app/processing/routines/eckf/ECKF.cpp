#include "ECKF.h"

ECKF::ECKF() {
}

void ECKF::setup(int blockSize) {
}

void ECKF::one_block(const std::vector<double>& y_frame, double fs, double c,
                     double timeToWait, int nPeaks, int nSemitones,
                     std::vector<double>& f0, std::vector<double>& amp) {
    // Detect if current frame is silent.
    const bool silent_prev = silent_cur;
    silent_cur = is_silent(y_frame);
    spf.push_back(cur_spf);
    // If current frame is silent, then continue
    if (silent_cur) {
        flag = 0;
        return;
    }

    // Detect harmonic change (note change)
}
