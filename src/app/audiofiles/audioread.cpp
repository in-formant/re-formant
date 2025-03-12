#include <sndfile.h>

#include <iostream>

#include "audiofiles.h"

bool reformant::audiofiles::readFile(const std::string& filePath,
                                 std::vector<float>& data, int* sampleRate) {
    SF_INFO sfinfo = {};
    SNDFILE *sndfile = sf_open(filePath.c_str(), SFM_READ, &sfinfo);

    if (sndfile == nullptr) {
        std::cerr << "sndfile: error opening file:" << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }

    sf_command(sndfile, SFC_SET_SCALE_FLOAT_INT_READ, nullptr, SF_TRUE);

    const sf_count_t length = sf_seek(sndfile, 0, SEEK_END);

    sf_seek(sndfile, 0, SEEK_SET);

    std::vector<float> multichannelData(length * sfinfo.channels);
    sf_readf_float(sndfile, multichannelData.data(), length);

    if (int err = sf_close(sndfile); err != 0) {
        std::cerr << "sndfile: error closing file:" << sf_error_number(err)
                  << std::endl;
        return false;
    }

    *sampleRate = sfinfo.samplerate;

    // Mix down to mono.
    data.resize(length);
    for (int i = 0; i < length; ++i) {
        data[i] = 0.0;
        for (int ch = 0; ch < sfinfo.channels; ++ch) {
            data[i] += multichannelData[i * sfinfo.channels + ch];
        }
        data[i] /= sfinfo.channels;
    }

    return true;
}