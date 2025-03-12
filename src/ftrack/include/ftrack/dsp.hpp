#ifndef FTRACK_DSP_HPP
#define FTRACK_DSP_HPP

#include "ftrack/config.hpp"

namespace ftrack {
namespace dsp {
// Sum of Residual Harmonics (SRH) pitch estimation method

int_type SRH_numframes(int_type len, int_type Fs);

void SRH_EstimatePitch(const float_type* sig, int_type sig_len,
                       int_type Fs, int_type F0min, int_type F0max,
                       int_type* f0, float_type* SRHVal);

void SRH_PitchTracking(const float_type* sig, int_type sig_len,
                       int_type Fs, int_type F0min, int_type F0max,
                       int_type* f0, int_type* VUVDecisions, float_type* SRHVal);

// SEDREAMS method for estimating Glottal Closure Instants (GCI)

std::vector<int_type> SEDREAMS_GCIDetection(const float_type* sig, int_type sig_len,
                                            int_type Fs, float_type F0mean);

// Median (sorts the input)

float_type median(std::vector<float_type>& v);

// In-place pre-emphasis

void preemphasis(float_type* x, int_type len, float_type preemp, float_type prev_x);

// Linear Prediction

void lpc(const float_type* sig, int_type sig_len, int_type order,
         float_type* lpca, float_type* energy);

void lpcresidual(const float_type* sig, int_type sig_len,
                 int_type L, int_type shift, int_type order,
                 float_type* res);

// Windowing functions

ArrayXf hamming(int_type len);

ArrayXf hann(int_type len);

ArrayXf blackman(int_type len);

ArrayXf blackmanharris(int_type len);

ArrayXf flattopwin(int_type len);

// FFTs

ArrayXcf fft(const ArrayXcf& x, int_type N);

ArrayXcf ifft(const ArrayXcf& x, int_type N);

ArrayXcf rfft(const ArrayXf& x, int_type N);

ArrayXf irfft(const ArrayXcf& x, int_type N);

// FIR/IIR filtering
void filter(const float_type* x, int_type len_x,
            const float_type* b, int_type len_b,
            const float_type* a, int_type len_a,
            float_type* y, float_type* Z = nullptr);

// FIR
void filter(const float_type* x, int_type len_x,
            const float_type* b, int_type len_b,
            float_type* y, float_type* Z = nullptr);
} // namespace dsp
} // namespace ftrack

#endif //FTRACK_DSP_HPP
