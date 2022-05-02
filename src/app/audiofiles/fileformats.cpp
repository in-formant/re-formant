#include <sndfile.h>

#include <numeric>
#include <set>

#include "audiofiles.h"

using namespace reformant;
using namespace audiofiles;

static void replaceAll(const std::string& str, const std::string& from,
                       const std::string& to);
static std::string normalizeName(const std::string& name);

static std::vector<AudioFileHeader> s_headers;
static std::vector<AudioFileEncoding> s_encodings;

static std::vector<std::string> s_extensions;
static std::string s_readFilter;
static std::string s_writeFilter;

const std::vector<AudioFileHeader>& audiofiles::getFormatHeaders() {
    if (!s_headers.empty()) {
        return s_headers;
    }

    int count;
    sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &count, sizeof(count));

    SF_FORMAT_INFO info;

    for (int i = 0; i < count; ++i) {
        memset(&info, 0, sizeof(info));
        info.format = i;

        sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &info, sizeof(info));

        AudioFileHeader header;

        header.type = info.format & SF_FORMAT_TYPEMASK;
        header.name = info.name;
        header.shortName = header.name.substr(0, header.name.find(' '));
        header.extension = info.extension;

        s_headers.push_back(header);
    }

    return s_headers;
}

const std::vector<AudioFileEncoding>& audiofiles::getFormatEncodings() {
    if (!s_encodings.empty()) {
        return s_encodings;
    }

    int count;
    sf_command(nullptr, SFC_GET_FORMAT_SUBTYPE_COUNT, &count, sizeof(count));

    SF_FORMAT_INFO info;

    for (int i = 0; i < count; ++i) {
        memset(&info, 0, sizeof(info));
        info.format = i;

        sf_command(nullptr, SFC_GET_FORMAT_SUBTYPE, &info, sizeof(info));

        int subtype = info.format & SF_FORMAT_SUBMASK;
        std::string name = normalizeName(info.name);

        s_encodings.push_back({subtype, name});
    }

    return s_encodings;
}

const std::string& audiofiles::getReadFilter() {
    if (!s_readFilter.empty()) {
        return s_readFilter;
    }

    std::vector<std::string> extensions;

    for (const auto& header : getFormatHeaders()) {
        extensions.push_back("." + header.extension);
    }

    // Some other extensions that are often sound files but aren't included by
    // libsndfile.
    extensions.insert(extensions.end(),
                      {".aif", ".ircam", ".snd", ".svx", ".svx8", ".svx16",
                       ".mp1", ".mp2", ".mp3"});

    s_readFilter = "Audio files {";
    s_readFilter += std::accumulate(
        std::next(extensions.begin()), extensions.end(), extensions[0],
        [](const auto& a, const auto& b) { return a + "," + b; });
    s_readFilter += "}";

    return s_readFilter;
}

template <typename T>
inline bool contains(const std::vector<T>& list, T elem, int* index) {
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == elem) {
            *index = i;
            return true;
        }
    }
    return false;
}

std::vector<AudioFileFormat> audiofiles::getCompatibleFormats(
    const int sampleRate) {
    std::vector<AudioFileFormat> formats;

    SF_INFO info;
    memset(&info, 0, sizeof(info));

    info.channels = 1;
    info.samplerate = sampleRate;

    // Header type whitelist:
    static const std::vector<int> headerWhitelist{
        SF_FORMAT_WAV,  SF_FORMAT_AIFF, SF_FORMAT_RAW,
        SF_FORMAT_FLAC, SF_FORMAT_OGG,  SF_FORMAT_MPEG};
    static const std::vector<const char*> headerNames{"WAV",  "AIFF", "RAW",
                                                      "FLAC", "OGG",  "MP3"};

    // Subtype preference / whitelist:
    static const std::vector<int> encodingPriority{
        SF_FORMAT_PCM_16, SF_FORMAT_VORBIS, SF_FORMAT_OPUS,
        SF_FORMAT_MPEG_LAYER_III};
    static const std::vector<const char*> encodingNames{"16-bit PCM", "Vorbis",
                                                        "Opus", nullptr};

    constexpr size_t bufsz = 32;
    char name[bufsz];

    for (const auto& header : getFormatHeaders()) {
        int i;
        if (!contains(headerWhitelist, header.type, &i)) continue;

        // For each encoding in the priority list, check if it's supported.
        for (int j = 0; j < encodingPriority.size(); ++j) {
            info.format = header.type | encodingPriority[j];

            // Special case for WAV + MPEG, for some reason it doesn't actually
            // work.
            if (header.type == SF_FORMAT_WAV &&
                encodingPriority[j] == SF_FORMAT_MPEG_LAYER_III) {
                continue;
            }

            if (sf_format_check(&info)) {
                std::string extension = header.extension;
                if (header.type == SF_FORMAT_MPEG) {
                    extension = "mp3";
                } else if (header.type == SF_FORMAT_OGG) {
                    extension = "ogg";
                } else {
                    extension = header.extension;
                }

                if (encodingNames[j] != nullptr) {
                    snprintf(name, bufsz, "%s %s", headerNames[i],
                             encodingNames[j]);
                } else {
                    snprintf(name, bufsz, "%s", headerNames[i]);
                }
                name[bufsz - 1] = '\0';

                formats.push_back({info.format, name, extension});
            }
        }
    }

    return formats;
}

std::string audiofiles::getWriteFilter(const int sampleRate) {
    const auto formats = getCompatibleFormats(sampleRate);

    std::string filter;

    constexpr size_t bufsz = 80;
    char filterPiece[bufsz];

    for (int i = 0; i < formats.size(); ++i) {
        snprintf(filterPiece, bufsz, "%s (*.%s){.%s}", formats[i].name.c_str(),
                 formats[i].extension.c_str(), formats[i].extension.c_str());
        filterPiece[bufsz - 1] = '\0';
        filter += filterPiece;
        filter += ",";
    }

    return filter;
}

// utility

void replaceAll(std::string& str, const std::string& from,
                const std::string& to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

std::string normalizeName(const std::string& name) {
    std::string n = name;
    replaceAll(n, "8 bit", "8-bit");
    replaceAll(n, "16 bit", "16-bit");
    replaceAll(n, "24 bit", "24-bit");
    replaceAll(n, "32 bit", "32-bit");
    replaceAll(n, "64 bit", "64-bit");
    return n;
}
