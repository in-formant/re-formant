#include "denoiser.h"

#include <rnnoise.h>

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

std::vector<float> Denoiser::process(const std::vector<float>& in) {
    if (in.size() < rnnoise_get_frame_size()) {
        throw new DenoiserError("Denoising frame too small");
    }

    std::vector<float> out(in);

    // Convert to signed 16-bit range
    for (float& x : out) x *= std::numeric_limits<int16_t>::max();

    rnnoise_process_frame(_p->st, out.data(), out.data());

    // Convert back to [-1,1] range
    for (float& x : out) x /= std::numeric_limits<int16_t>::max();

    return out;
}

DenoiserError::DenoiserError(const char* msg) : msg(msg) {}

const char* DenoiserError::what() const noexcept { return msg; }
