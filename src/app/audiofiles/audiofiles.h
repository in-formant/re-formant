#ifndef REFORMANT_AUDIOFILE_AUDIOFILES_H
#define REFORMANT_AUDIOFILE_AUDIOFILES_H

#include <string>
#include <vector>

namespace reformant {
namespace audiofiles {

struct AudioFileHeader {
    int type;
    std::string name;
    std::string shortName;
    std::string extension;
};

const std::vector<AudioFileHeader>& getFormatHeaders();

struct AudioFileEncoding {
    int subtype;
    std::string name;
};

const std::vector<AudioFileEncoding>& getFormatEncodings();

bool readFile(const std::string& filePath, std::vector<float>& data,
              int* sampleRate);

const std::string& getReadFilter();

struct AudioFileFormat {
    int format;
    std::string name;
    std::string extension;
};

bool writeFile(const std::string& filePath, const AudioFileFormat& format,
               const std::vector<float>& data, const int sampleRate);

std::vector<AudioFileFormat> getCompatibleFormats(int sampleRate);
std::string getWriteFilter(int sampleRate);

}  // namespace audiofiles
}  // namespace reformant

/*
#include <string>
#include <utility>
#include <vector>

namespace reformant {

struct AudioFormats {
    std::vector<AudioFileHeader> headers;
    std::string filter;
};

AudioFormats getSupportedAudioFormats();

bool writeAudioFile(const std::string& filePath, const int format,
                    const std::vector<float>& data, const int sampleRate);

std::string getAudioFileReadFilter();

bool readAudioFile(const std::string& filePath, std::vector<float>& data,
                   int* sampleRate);

}  // namespace reformant
*/

#endif  // REFORMANT_AUDIOFILE_AUDIOFILES_H