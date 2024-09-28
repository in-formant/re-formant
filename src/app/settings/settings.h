#ifndef REFORMANT_SETTINGS_SETTINGS_H
#define REFORMANT_SETTINGS_SETTINGS_H

#include <map>
#include <string>
#include <cstdint>

#include "../audio/audiodevices.h"

namespace reformant {
using SettingsMap = std::map<std::string, std::string>;

struct SettingsBackend {
    using ReadFunc = void (*)(const std::string&, SettingsMap&);
    using WriteFunc = void (*)(const std::string&, const SettingsMap&);

    SettingsBackend();

    SettingsBackend(ReadFunc read, WriteFunc write);

    ReadFunc read;
    WriteFunc write;
};

class Settings {
public:
    Settings();

    Settings(SettingsBackend backend);

    void load();

    void save();

    bool showAudioSettings();

    void setShowAudioSettings(bool bFlag);

    bool showDisplaySettings();

    void setShowDisplaySettings(bool bFlag);

    bool showProfiler();

    void setShowProfiler(bool bFlag);

    void spectrumPlotRatios(float ratios[2]);

    void setSpectrumPlotRatios(const float ratios[2]);

    int spectrumFreqScale();

    void setSpectrumFreqScale(int scale);

    double spectrumFreqMin();

    void setSpectrumFreqMin(double freq);

    double spectrumFreqMax();

    void setSpectrumFreqMax(double freq);

    double spectrumMinDb();

    void setSpectrumMinDb(double db);

    double spectrumMaxDb();

    void setSpectrumMaxDb(double db);

    void pitchColor(float rgb[3]);

    void setPitchColor(const float rgb[3]);

    void formantColor(float rgb[3]);

    void setFormantColor(const float rgb[3]);

    bool doStartRecordingOnLaunch();

    void setStartRecordingOnLaunch(bool bFlag);

    bool doNoiseReduction();

    void setNoiseReduction(bool bFlag);

    int audioHostApi();

    void setAudioHostApi(int hostApiType);

    std::string inputDeviceName();

    void setInputDeviceName(const std::string& deviceName);

    std::string outputDeviceName();

    void setOutputDeviceName(const std::string& deviceName);

    int trackSampleRate();

    void setTrackSampleRate(int sampleRate);

    int fftLength();

    void setFftLength(int nfft);

    uint64_t maxSpectrogramMemory();

    void setMaxSpectrogramMemory(uint64_t mem);

private:
    // Wrapper to save and return value in one line.
    template <typename T>
    T save(T value) {
        save();
        return value;
    }

    SettingsBackend m_backend;
    SettingsMap m_map;
};
} // namespace reformant

#endif  // REFORMANT_SETTINGS_SETTINGS_H
