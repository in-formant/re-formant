#include <sndfile.h>

#include <algorithm>
#include <set>

#include "audiofiles.h"

reformant::AudioFormats reformant::getSupportedAudioFormats() {
    // List supported file formats by this build of sndfile.
    SF_FORMAT_INFO info;
    SF_INFO sfinfo;
    int majorCount;

    sfinfo.samplerate = 48000;
    sfinfo.channels = 1;

    sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &majorCount, sizeof(int));

    std::vector<int> indexList;
    std::string filter;

    char filterPiece[64];

    for (int m = 0; m < majorCount; m++) {
        info.format = m;
        sf_command(NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof(info));

        sfinfo.format = (info.format & SF_FORMAT_TYPEMASK) | SF_FORMAT_PCM_16;

        if (sf_format_check(&sfinfo)) {
            indexList.push_back(info.format);
            snprintf(filterPiece, 64, "%s (*.%s){%s}", info.name,
                     info.extension, info.extension);
            filterPiece[63] = '\0';
            filter += filterPiece;
            if (m < majorCount - 1) {
                filter += ",";
            }
        }
    }

    return {indexList, filter};
}

std::string reformant::getAudioFileReadFilter() {
    // List supported file formats by this build of sndfile.
    SF_FORMAT_INFO info;
    SF_INFO sfinfo;
    int majorCount;

    sfinfo.samplerate = 48000;
    sfinfo.channels = 1;

    sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &majorCount, sizeof(int));

    std::set<std::string> extensions;

    for (int m = 0; m < majorCount; m++) {
        info.format = m;
        sf_command(NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof(info));

        sfinfo.format = (info.format & SF_FORMAT_TYPEMASK) | SF_FORMAT_PCM_16;

        if (sf_format_check(&sfinfo)) {
            extensions.emplace(info.extension);
        }
    }

    std::string filter = "Audio files {";

    for (auto it = extensions.begin(); it != extensions.end(); ++it) {
        filter += ".";
        filter += *it;
        if (it != std::prev(extensions.end())) {
            filter += ",";
        }
    }

    filter += "}";

    return filter;
}