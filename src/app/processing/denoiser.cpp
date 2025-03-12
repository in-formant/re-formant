#include "denoiser.h"

#include <rnnoise.h>

#include <cstdint>
#include <limits>

using namespace reformant;

struct reformant::DenoiserPrivate {
    DenoiseState* st;
};

Denoiser::Denoiser() {
    _p = new DenoiserPrivate;
    _p->st = rnnoise_create(nullptr);
}

Denoiser::~Denoiser() {
    rnnoise_destroy(_p->st);
    delete _p;
}

int Denoiser::frameSize() { return rnnoise_get_frame_size(); }

std::vector<float> Denoiser::process(const std::vector<float>& in) {
    if (in.size() < rnnoise_get_frame_size()) {
        throw new DenoiserError("Denoising frame too small");
    }

    _tmp.resize(in.size());
    std::copy(in.begin(), in.end(), _tmp.begin());

    // Convert to signed 16-bit range
    for (float& x : _tmp) x *= std::numeric_limits<int16_t>::max();

    rnnoise_process_frame(_p->st, _tmp.data(), _tmp.data());

    // Convert back to [-1,1] range
    for (float& x : _tmp) x /= std::numeric_limits<int16_t>::max();

    return _tmp;
}

DenoiserError::DenoiserError(const char* msg) : msg(msg) {}

const char* DenoiserError::what() const noexcept { return msg; }
