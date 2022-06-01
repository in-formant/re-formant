#include "resampler.h"

#include <speex_resampler.h>

#include <cstdarg>
#include <cstdio>

using namespace reformant;

struct reformant::ResamplerPrivate {
    int err;
    SpeexResamplerState* st;
};

Resampler::Resampler(const int quality, const bool skipZeros)
    : m_isValid(false), m_quality(quality), m_skipZeros(skipZeros) {
    _p = new ResamplerPrivate;
}

Resampler::Resampler(const double inFs, const double outFs, const int quality,
                     const bool skipZeros)
    : m_isValid(false),
      m_inputRate(inFs),
      m_outputRate(outFs),
      m_quality(quality),
      m_skipZeros(skipZeros) {
    _p = new ResamplerPrivate;
    createResampler();
}

Resampler::~Resampler() {
    if (m_isValid) {
        speex_resampler_destroy(_p->st);
    }
    delete _p;
}

bool Resampler::isValid() const { return m_isValid; }

void Resampler::setRate(const double inFs, const double outFs) {
    m_inputRate = inFs;
    m_outputRate = outFs;
    if (m_isValid) {
        _p->err = speex_resampler_set_rate(_p->st, inFs, outFs);
        if (_p->err != 0) {
            throw new ResamplerError("Speex set ratio error: %s",
                                     speex_resampler_strerror(_p->err));
        }
    } else {
        createResampler();
    }
}

void Resampler::reset() {
    if (!m_isValid) throw new ResamplerError("Resampler is invalid");

    _p->err = speex_resampler_reset_mem(_p->st);
    if (_p->err != 0) {
        throw new ResamplerError("Speex reset mem error: %s",
                                 speex_resampler_strerror(_p->err));
    }
}

void Resampler::skipZeros() {
    if (!m_isValid) throw new ResamplerError("Resampler is invalid");

    _p->err = speex_resampler_skip_zeros(_p->st);
    if (_p->err != 0) {
        throw new ResamplerError("Speex skip zeros error: %s",
                                 speex_resampler_strerror(_p->err));
    }
}

int Resampler::inputLatency() const { return speex_resampler_get_input_latency(_p->st); }

int Resampler::outputLatency() const {
    return speex_resampler_get_output_latency(_p->st);
}

void Resampler::process(std::vector<float>& out, const std::vector<float>& data,
                        const int offset, int length) {
    if (!m_isValid) throw new ResamplerError("Resampler is invalid");

    if (data.empty()) {
        out.clear();
        return;
    }

    if (offset < 0 || offset >= data.size())
        throw new ResamplerError("Input vector offset out of bounds");

    if (length < 0) {
        length = (int)data.size() - offset;
    }

    uint32_t ilen = length;
    uint32_t olen;
    _p->err = speex_resampler_get_expected_output_frame_count(_p->st, ilen, &olen);
    if (_p->err != 0) {
        throw new ResamplerError("Speex expected output frame count error: %s",
                                 speex_resampler_strerror(_p->err));
    }

    out.resize(olen);

    _p->err = speex_resampler_process_float(_p->st, 0, data.data() + offset, &ilen,
                                            out.data(), &olen);
    if (_p->err != 0) {
        throw new ResamplerError("Speex process error: %s",
                                 speex_resampler_strerror(_p->err));
    }

    out.resize(olen);
}

std::vector<float> Resampler::process(const std::vector<float>& data, const int offset,
                                      const int length) {
    std::vector<float> out;
    process(out, data, offset, length);
    return out;
}

int Resampler::requiredInputFrames(const int outputLength) const {
    if (!m_isValid) throw new ResamplerError("Resampler is invalid");

    uint32_t ilen;
    uint32_t olen = outputLength;
    _p->err = speex_resampler_get_required_input_frame_count(_p->st, olen, &ilen);
    if (_p->err != 0) {
        throw new ResamplerError("Speex required input frame count error: %s",
                                 speex_resampler_strerror(_p->err));
    }

    return (int)ilen;
}

// utils

void Resampler::createResampler() {
    if (m_isValid) throw new ResamplerError("Can't create same resampler twice");

    _p->st = speex_resampler_init(1, m_inputRate, m_outputRate, m_quality, &_p->err);
    if (_p->err != 0) {
        throw new ResamplerError("Speex new error: %s",
                                 speex_resampler_strerror(_p->err));
    }
    m_isValid = true;

    if (m_skipZeros) {
        skipZeros();
    }
}

ResamplerError::ResamplerError(const char* format, ...) {
    va_list list;
    va_start(list, format);
    vsnprintf(msg, msgsz, format, list);
    va_end(list);
    msg[msgsz - 1] = '\0';
}

const char* ResamplerError::what() const noexcept { return msg; }