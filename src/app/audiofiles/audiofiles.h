#ifndef REFORMANT_AUDIOFILE_AUDIOFILES_H
#define REFORMANT_AUDIOFILE_AUDIOFILES_H

#include <string>
#include <utility>
#include <vector>

namespace reformant {

struct AudioFormats {
    std::vector<int> formats;
    std::string filter;
};

AudioFormats getSupportedAudioFormats();

bool writeAudioFile(const std::string& filePath, const int format,
                    const std::vector<float>& data, const int sampleRate);

std::string getAudioFileReadFilter();

bool readAudioFile(const std::string& filePath, std::vector<float>& data,
                   int* sampleRate);

}  // namespace reformant

#endif  // REFORMANT_AUDIOFILE_AUDIOFILES_H