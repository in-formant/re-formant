#include <sndfile.h>

#include <iostream>

#include "audiofiles.h"

bool reformant::writeAudioFile(const std::string& filePath, const int format,
                               const std::vector<float>& data,
                               const int sampleRate) {
    SNDFILE* sndfile;
    int mode = SFM_WRITE;

    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    sfinfo.samplerate = sampleRate;
    sfinfo.channels = 1;
    sfinfo.format = (format & SF_FORMAT_TYPEMASK) | SF_FORMAT_PCM_16;

    sndfile = sf_open(filePath.c_str(), mode, &sfinfo);

    if (sndfile == nullptr) {
        std::cerr << "sndfile: error opening file:" << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }

    sf_command(sndfile, SFC_SET_SCALE_INT_FLOAT_WRITE, nullptr, SF_TRUE);

    sf_writef_float(sndfile, data.data(), data.size());

    sf_write_sync(sndfile);

    int err = sf_close(sndfile);
    if (err != 0) {
        std::cerr << "sndfile: error closing file:" << sf_error_number(err)
                  << std::endl;
        return false;
    }

    return true;
}