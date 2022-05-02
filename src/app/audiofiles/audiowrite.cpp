#include <sndfile.h>

#include <iostream>

#include "audiofiles.h"

bool reformant::audiofiles::writeFile(const std::string& filePath,
                                      const AudioFileFormat& format,
                                      const std::vector<float>& data,
                                      const int sampleRate) {
    SNDFILE* sndfile;
    int mode = SFM_WRITE;

    SF_INFO info;
    memset(&info, 0, sizeof(SF_INFO));

    info.channels = 1;
    info.samplerate = sampleRate;
    info.format = format.format;

    sndfile = sf_open(filePath.c_str(), mode, &info);

    if (sndfile == nullptr) {
        std::cerr << "sndfile: error opening file:" << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }

    double compression = 0.5;
    sf_command(sndfile, SFC_SET_COMPRESSION_LEVEL, &compression,
               sizeof(double));

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