#include <sndfile.h>

#include <iostream>

#include "audiofiles.h"

bool reformant::readAudioFile(const std::string& filePath,
                              std::vector<float>& data, int* sampleRate) {
    SNDFILE* sndfile;
    int mode = SFM_READ;

    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    sndfile = sf_open(filePath.c_str(), mode, &sfinfo);

    if (sndfile == nullptr) {
        std::cerr << "sndfile: error opening file:" << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }

    sf_command(sndfile, SFC_SET_SCALE_FLOAT_INT_READ, nullptr, SF_TRUE);

    sf_count_t length = sf_seek(sndfile, 0, SEEK_END);

    sf_seek(sndfile, 0, SEEK_SET);

    std::vector<float> multichannelData(length * sfinfo.channels);
    sf_readf_float(sndfile, multichannelData.data(), length);

    int err = sf_close(sndfile);
    if (err != 0) {
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